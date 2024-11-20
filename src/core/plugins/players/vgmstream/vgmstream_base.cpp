/**
 *
 * @file
 *
 * @brief  vgmstream-based formats support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/container.h"
#include "module/players/properties_helper.h"

#include "binary/container_base.h"
#include "binary/crc.h"
#include "binary/format.h"
#include "binary/format_factories.h"
#include "binary/view.h"
#include "core/plugin_attrs.h"
#include "debug/log.h"
#include "formats/multitrack.h"
#include "math/numeric.h"
#include "module/additional_files.h"
#include "module/holder.h"
#include "module/information.h"
#include "module/renderer.h"
#include "module/state.h"
#include "parameters/container.h"
#include "sound/chunk.h"
#include "sound/receiver.h"
#include "sound/resampler.h"
#include "strings/array.h"
#include "strings/sanitize.h"
#include "time/duration.h"
#include "time/instant.h"

#include "contract.h"
#include "error.h"
#include "make_ptr.h"
#include "pointers.h"
#include "static_string.h"
#include "types.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <iterator>
#include <memory>
#include <set>
#include <utility>

extern "C"
{
  // clang-format off
#include "3rdparty/vgmstream/config.h" // IWYU pragma: keep
#include "3rdparty/vgmstream/base/plugins.h"
#include "3rdparty/vgmstream/vgmstream.h"
#include "3rdparty/vgmstream/streamfile.h"
#include "3rdparty/vgmstream/streamtypes.h"
#include "3rdparty/vgmstream/vgmstream_types.h"
  // clang-format on
}

namespace Module::VGMStream
{
  const Debug::Stream Dbg("Core::VGMStream");

  class Vfs
  {
  public:
    using Ptr = std::shared_ptr<Vfs>;

    Vfs(StringView rootName, Binary::Data::Ptr rootData)
    {
      Content.emplace_back(rootName, std::move(rootData));
    }

    StringView Root() const
    {
      return Content.front().first;
    }

    Binary::View Request(StringView name)
    {
      const auto it =
          std::find_if(Content.begin(), Content.end(), [name](const auto& entry) { return name == entry.first; });
      if (it != Content.end() && it->second)
      {
        Dbg("Found {}", name);
        return *it->second;
      }
      else
      {
        Dbg("Not found {}", name);
        if (it == Content.end())
        {
          Content.emplace_back(name, Binary::Data::Ptr());
        }
        return {nullptr, 0};
      }
    }

    bool HasUnresolved() const
    {
      return std::any_of(Content.begin(), Content.end(), [](const auto& entry) { return !entry.second; });
    }

    Strings::Array GetUnresolved() const
    {
      Strings::Array result;
      for (const auto& entry : Content)
      {
        if (!entry.second)
        {
          result.emplace_back(entry.first);
        }
      }
      Dbg("Up to {} unresolved files", result.size());
      return result;
    }

    void Resolve(StringView name, Binary::Data::Ptr data)
    {
      const auto it =
          std::find_if(Content.begin(), Content.end(), [name](const auto& entry) { return name == entry.first; });
      Require(it != Content.end());
      it->second = std::move(data);
    }

    void ClearUnresolved()
    {
      const auto end = std::remove_if(Content.begin(), Content.end(), [](const auto& entry) { return !entry.second; });
      Content.resize(std::distance(Content.begin(), end));
    }

    bool IsMultifile() const
    {
      return Content.size() > 1;
    }

  private:
    // enumerate in order of appearance - don't use std::map
    using Storage = std::vector<std::pair<String, Binary::Data::Ptr>>;
    Storage Content;
  };

  class MemoryStream : public STREAMFILE
  {
  public:
    explicit MemoryStream(const Vfs::Ptr& vfs)
      : MemoryStream(vfs, vfs->Root(), vfs->Request(vfs->Root()))
    {}

  private:
    MemoryStream(Vfs::Ptr vfs, StringView filename, Binary::View raw)
      : STREAMFILE()
      , Fs(std::move(vfs))
      , Filename(filename)
      , Raw(raw)
    {
      read = &Read;
      get_size = &Length;
      get_offset = &Tell;
      get_name = &GetName;
      open = &Open;
      close = &Close;
      stream_index = 0;
      Require(!!Raw);
    }

    static MemoryStream* Cast(void* ctx)
    {
      return static_cast<MemoryStream*>(ctx);
    }

    static size_t Read(STREAMFILE* ctx, uint8_t* dst, offv_t offset, size_t length)
    {
      auto* self = Cast(ctx);
      if (const auto sub = self->Raw.SubView(offset, length))
      {
        std::memcpy(dst, sub.Start(), sub.Size());
        self->Position = offset + sub.Size();
        return sub.Size();
      }
      else
      {
        return 0;
      }
    }

    static size_t Length(STREAMFILE* ctx)
    {
      return Cast(ctx)->Raw.Size();
    }

    static offv_t Tell(STREAMFILE* ctx)
    {
      return Cast(ctx)->Position;
    }

    static void GetName(STREAMFILE* ctx, char* name, size_t length)
    {
      const auto& filename = Cast(ctx)->Filename;
      strncpy(name, filename.c_str(), length);
      name[length - 1] = '\0';
    }

    static STREAMFILE* Open(STREAMFILE* ctx, const char* const filename, size_t /*buffersize*/)
    {
      if (!filename)
      {
        return nullptr;
      }
      auto* self = Cast(ctx);
      const StringView name(filename);
      if (auto blob = self->Fs->Request(name))
      {
        auto* result = new MemoryStream(self->Fs, name, blob);
        result->stream_index = self->stream_index;
        return result;
      }
      else
      {
        return nullptr;
      }
    }

    static void Close(STREAMFILE* ctx)
    {
      delete Cast(ctx);
    }

  private:
    const Vfs::Ptr Fs;
    const String Filename;
    const Binary::View Raw;  // owned by Fs
    std::size_t Position = 0;
  };

  const Time::Milliseconds FRAME_DURATION(20);

  using VGMStreamPtr = std::shared_ptr<VGMSTREAM>;

  class State : public Module::State
  {
  public:
    explicit State(VGMStreamPtr stream)
      : Stream(std::move(stream))
    {}

    Time::AtMillisecond At() const override
    {
      return Time::AtMillisecond() + Time::Milliseconds::FromRatio(Stream->current_sample, Stream->sample_rate);
    }

    Time::Milliseconds Total() const override
    {
      return Time::Milliseconds::FromRatio(Stream->pstate.play_duration, Stream->sample_rate);
    }

    uint_t LoopCount() const override
    {
      return Stream->loop_count;
    }

  private:
    const VGMStreamPtr Stream;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(VGMStreamPtr tune, uint_t samplerate)
      : Tune(std::move(tune))
      , Status(MakePtr<State>(Tune))
      , SamplesPerFrame(FRAME_DURATION.Get() * Tune->sample_rate / FRAME_DURATION.PER_SECOND)
      , Target(Sound::CreateResampler(Tune->sample_rate, samplerate))
      , Channels(Tune->channels)
    {
      ::vgmstream_mixing_autodownmix(Tune.get(), Sound::Sample::CHANNELS);
      ::vgmstream_mixing_enable(Tune.get(), SamplesPerFrame, nullptr, &Channels);
      Dbg("Rendering {}Hz/{}ch -> {}Hz/{}ch", Tune->sample_rate, Tune->channels, samplerate, Channels);
    }

    State::Ptr GetState() const override
    {
      return Status;
    }

    Sound::Chunk Render() override
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
      const auto toRender = std::min<uint_t>(SamplesPerFrame, Tune->num_samples - Tune->current_sample);
      const auto multichannelSamples = toRender * Tune->channels;
      // Downmixing is performed after multichannel rendering
      Sound::Chunk result(Math::Align<uint_t>(multichannelSamples, Sound::Sample::CHANNELS) / Sound::Sample::CHANNELS);
      const auto current_before = Tune->current_sample;
      const auto done = ::render_vgmstream(safe_ptr_cast<sample_t*>(result.data()), toRender, Tune.get());
      if (Tune->current_sample == Tune->num_samples || Tune->current_sample == current_before)
      {
        // TODO: take into account loop_end_sample
        const auto loop_count = Tune->loop_count;
        DoSeek(Tune->loop_start_sample);
        Tune->loop_count = loop_count + 1;
      }
      result.resize(done);
      if (Channels == 1)
      {
        auto* mono = safe_ptr_cast<Sound::Sample::Type*>(result.data());
        auto* stereo = result.data();
        for (int pos = done - 1; pos >= 0; --pos)
        {
          const auto in = mono[pos];
          stereo[pos] = Sound::Sample(in, in);
        }
      }
      Tune->pstate.play_duration += done;
      return Target->Apply(std::move(result));
    }

    void Reset() override
    {
      ::reset_vgmstream(Tune.get());
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      const auto samples = uint64_t(Tune->sample_rate) * request.Get() / request.PER_SECOND;
      DoSeek(samples);
    }

  private:
    void DoSeek(uint_t samples)
    {
      // Keep total playback history
      const auto play_duration = Tune->pstate.play_duration;
      ::seek_vgmstream(Tune.get(), samples);
      Tune->pstate.play_duration = play_duration;
    }

  private:
    const VGMStreamPtr Tune;
    const State::Ptr Status;
    const uint_t SamplesPerFrame;
    const Sound::Converter::Ptr Target;
    int Channels;
  };

  class Information : public Module::Information
  {
  public:
    explicit Information(const VGMStreamPtr& stream)
      : Total(stream->num_samples)
      , LoopStart(stream->loop_start_sample)
      , Samplerate(stream->sample_rate)
    {}

    Time::Milliseconds Duration() const override
    {
      return Time::Milliseconds::FromRatio(Total, Samplerate);
    }

    Time::Milliseconds LoopDuration() const override
    {
      return Time::Milliseconds::FromRatio(Total - LoopStart, Samplerate);
    }

  private:
    const int Total;
    const int LoopStart;
    const int Samplerate;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Vfs::Ptr model, VGMStreamPtr stream, Parameters::Accessor::Ptr props)
      : Model(std::move(model))
      , Stream(std::move(stream))
      , Info(MakePtr<Information>(Stream))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      try
      {
        return MakePtr<Renderer>(GetStream(), samplerate);
      }
      catch (const std::exception& e)
      {
        throw Error(THIS_LINE, e.what());
      }
    }

  private:
    VGMStreamPtr GetStream() const
    {
      if (!Stream)
      {
        Require(!!Stream);  // TODO
      }
      return {std::move(Stream)};
    }

  private:
    const Vfs::Ptr Model;
    mutable VGMStreamPtr Stream;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  VGMStreamPtr TryOpenStream(const Vfs::Ptr& vfs, int subtrackIndex = -1)
  {
    // Streams are reopened for further seeking and partial decoding
    MemoryStream stream(vfs);
    stream.stream_index = subtrackIndex + 1;  // 1-based really
    if (auto result = VGMStreamPtr(::init_vgmstream_from_STREAMFILE(&stream), &::close_vgmstream))
    {
      Dbg("Found stream with {} streams, {} samples at {}Hz", result->num_streams, result->num_samples,
          result->sample_rate);
      if ((subtrackIndex == -1 && result->num_streams > 1) || result->num_samples >= result->sample_rate / 100)
      {
        return result;
      }
    }
    return {};
  }

  Module::Holder::Ptr TryCreateModule(Vfs::Ptr vfs, Parameters::Container::Ptr properties, int subtrackIndex = -1)
  {
    if (auto stream = TryOpenStream(vfs, subtrackIndex))
    {
      // Some invariants
      const auto singleTrackModule = subtrackIndex == -1 && stream->num_streams == 0;
      const auto multiTrackModule = subtrackIndex >= 0 && stream->num_streams > subtrackIndex;
      const auto singleTrackMultiFileModule = stream->coding_type != coding_FFmpeg && vfs->IsMultifile()
                                              && subtrackIndex == -1 && stream->num_streams == 1;
      const auto singleTrackFFMpegModule = stream->coding_type == coding_FFmpeg && subtrackIndex == -1
                                           && stream->num_streams == 1;
      Require(1 == singleTrackModule + multiTrackModule + singleTrackMultiFileModule + singleTrackFFMpegModule);
      PropertiesHelper props(*properties);
      props.SetTitle(Strings::Sanitize(stream->stream_name));
      {
        std::array<char, 1024> buf{0};
        ::describe_vgmstream(stream.get(), buf.data(), buf.size());
        props.SetComment(Strings::SanitizeMultiline(buf.data()));
      }
      return MakePtr<Holder>(std::move(vfs), std::move(stream), std::move(properties));
    }
    else
    {
      return {};
    }
  }

  class MultifileHolder
    : public Module::Holder
    , public Module::AdditionalFiles
  {
  public:
    MultifileHolder(Vfs::Ptr model, Parameters::Container::Ptr props)
      : Model(std::move(model))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return GetDelegate().GetModuleInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return GetDelegate().GetModuleProperties();
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      return GetDelegate().CreateRenderer(samplerate, std::move(params));
    }

    Strings::Array Enumerate() const override
    {
      return Model->GetUnresolved();
    }

    void Resolve(StringView name, Binary::Container::Ptr data) override
    {
      Dbg("Resolving dependency '{}'", name);
      Model->Resolve(name, std::move(data));
      TryCreateDelegate();
    }

  private:
    const Module::Holder& GetDelegate() const
    {
      if (!Delegate)
      {
        TryCreateDelegate();
      }
      return *Delegate;
    }

    void TryCreateDelegate() const
    {
      Delegate = TryCreateModule(Model, Properties);
      if (Delegate)
      {
        Dbg("All dependencies resolved");
        Model->ClearUnresolved();  // for any
      }
    }

  private:
    const Vfs::Ptr Model;
    const Parameters::Container::Ptr Properties;
    mutable Ptr Delegate;
  };

  enum class PluginType
  {
    SIMPLE,
    MULTIFILE,
    MULTITRACK
  };

  struct PluginDescription
  {
    const ZXTune::PluginId Id;
    const StringView Description;
    const char* const Suffix;  // for factory
    const StringView Format;
    const PluginType Type = PluginType::SIMPLE;
  };

  // up to 64
  constexpr auto CHANNELS8 = "01-40"_ss;
  constexpr auto CHANNELS16LE = CHANNELS8 + "00"_ss;
  constexpr auto CHANNELS16BE = "00"_ss + CHANNELS8;
  constexpr auto CHANNELS32LE = CHANNELS16LE + "00 00"_ss;
  constexpr auto CHANNELS32BE = "00 00"_ss + CHANNELS16BE;
  // up to 192000
  constexpr auto SAMPLERATE16LE = "? 01-ff"_ss;
  constexpr auto SAMPLERATE16BE = "01-ff ?"_ss;
  constexpr auto SAMPLERATE24BE = "00-02 ??"_ss;
  constexpr auto SAMPLERATE32LE = "?? 00-02 00"_ss;
  constexpr auto SAMPLERATE32BE = "00 00-02 ??"_ss;
  // up to 1B
  constexpr auto SAMPLES32LE = "??? 00-3b"_ss;
  constexpr auto SAMPLES32BE = "00-3b ???"_ss;

  constexpr auto ANY16 = "??"_ss;
  constexpr auto ANY32 = "????"_ss;

  namespace SingleTrack
  {
    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      explicit Decoder(const PluginDescription& desc)
        : Desc(desc)
        , Fmt(Binary::CreateMatchOnlyFormat(Desc.Format))
      {}

      StringView GetDescription() const override
      {
        return Desc.Description;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Fmt;
      }

      bool Check(Binary::View rawData) const override
      {
        return Fmt->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (Check(rawData))
        {
          const auto size = rawData.Size();
          auto data = rawData.GetSubcontainer(0, size);
          return Formats::Chiptune::CreateCalculatingCrcContainer(std::move(data), 0, size);
        }
        return {};
      }

    private:
      const PluginDescription& Desc;
      const Binary::Format::Ptr Fmt;
    };

    class Factory : public Module::ExternalParsingFactory
    {
    public:
      explicit Factory(const PluginDescription& desc)
        : Desc(desc)
      {}

      Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/,
                                       const Formats::Chiptune::Container& container,
                                       Parameters::Container::Ptr properties) const override
      {
        try
        {
          // assume all dump is used
          Dbg("Trying {}", Desc.Description);
          auto vfs = MakePtr<Vfs>(Desc.Suffix, container.GetSubcontainer(0, container.Size()));
          if (auto singlefile = TryCreateModule(vfs, properties))
          {
            return singlefile;
          }
          else if (Desc.Type == PluginType::MULTIFILE && vfs->HasUnresolved())
          {
            Dbg("Try to process as multifile module");
            return MakePtr<MultifileHolder>(std::move(vfs), std::move(properties));
          }
        }
        catch (const std::exception& e)
        {
          Dbg("Failed to create {}: {}", Desc.Id, e.what());
        }
        return {};
      }

    private:
      const PluginDescription& Desc;
    };
  }  // namespace SingleTrack
  namespace MultiTrack
  {
    class Container : public Binary::BaseContainer<Formats::Multitrack::Container>
    {
    public:
      Container(Binary::Container::Ptr delegate, uint_t totalTracks, uint_t trackIndex)
        : BaseContainer(std::move(delegate))
        , TotalTracks(totalTracks)
        , TrackIndex(trackIndex)
      {}

      uint_t FixedChecksum() const override
      {
        return Binary::Crc32(*Delegate);
      }

      uint_t TracksCount() const override
      {
        return TotalTracks;
      }

      uint_t StartTrackIndex() const override
      {
        return TrackIndex;
      }

    private:
      const uint_t TotalTracks;
      const uint_t TrackIndex;
    };

    class Decoder : public Formats::Multitrack::Decoder
    {
    public:
      explicit Decoder(const PluginDescription& desc)
        : Desc(desc)
        , Fmt(Binary::CreateMatchOnlyFormat(Desc.Format))
      {}

      StringView GetDescription() const override
      {
        return Desc.Description;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Fmt;
      }

      bool Check(Binary::View rawData) const override
      {
        return Fmt->Match(rawData);
      }

      Formats::Multitrack::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (Check(rawData))
        {
          auto data = rawData.GetSubcontainer(0, rawData.Size());
          auto vfs = MakePtr<Vfs>(Desc.Suffix, data);
          if (auto stream = TryOpenStream(vfs))
          {
            // Formats clashing
            if (stream->num_streams)
            {
              return MakePtr<Container>(std::move(data), stream->num_streams, 0);
            }
            else
            {
              return MakePtr<Container>(std::move(data), 1, -1);
            }
          }
        }
        return {};
      }

    private:
      const PluginDescription& Desc;
      const Binary::Format::Ptr Fmt;
    };

    class Factory : public Module::MultitrackFactory
    {
    public:
      explicit Factory(const PluginDescription& desc)
        : Desc(desc)
      {}

      Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/,
                                       const Formats::Multitrack::Container& container,
                                       Parameters::Container::Ptr properties) const override
      {
        try
        {
          Dbg("Trying {}", Desc.Description);
          // assume all dump is used
          auto vfs = MakePtr<Vfs>(Desc.Suffix, container.GetSubcontainer(0, container.Size()));
          if (auto singlefile = TryCreateModule(vfs, properties, container.StartTrackIndex()))
          {
            return singlefile;
          }
          else if (vfs->HasUnresolved())
          {
            Dbg("Unsupported multifile multitrack module...");
            return {};
          }
        }
        catch (const std::exception& e)
        {
          Dbg("Failed to create {}: {}", Desc.Id, e.what());
        }
        return {};
      }

    private:
      const PluginDescription& Desc;
    };
  }  // namespace MultiTrack

  using ZXTune::operator""_id;

  // clang-format off
  const PluginDescription PLUGINS[] =
  {
    //CRI
    // HCA, ~166.2k
    {
      "HCA"_id, "CRI HCA"sv, ".hca",
      "%x1001000 %x1000011 %x1000001 %x0000000" // +0 'HCA\0' (0x48434100), maybe 0x7f-masked
      "01|02|03 00|01|02|03"                    // +2 version 0x101, 0x102, 0x103, 0x200, 0x300
      "00-10 ?"                                 // +4 BE header size up to 0x1000

      "%x1100110 %x1101101 %x1110100 %x0000000" // +8 'fmt\0' (0x666d7400), maybe 0x7f-masked
      "01-10"_ss +                              // +c channels
      SAMPLERATE24BE +                          // +d samplerate
      ANY32 +                                   // +10 frame count
      ANY16 +                                   // +14 encoder delay
      ANY16 +                                   // +16 encoding padding
      "%x1100011 | %x1100100" // +18 'comp' (0x636f6d70) or 'dec\0' (0x64656300), maybe 0x7f-masked
      "%x1101111 | %x1100101"
      "%x1101101 | %x1100011"
      "%x1110000 | %x0000000"
      ""_ss
    },
    // ADX, ~62.9k+20.5k=~83.3k
    {
      "ADX"_id, "CRI ADX"sv, ".adx",
      "8000"           // +0 sign
      "??"             // +2 offset
      "02|03|04"       // +4 coding
      "?"              // +5 frame size
      "04"_ss +        // +6 bits per sample
      CHANNELS8 +      // +7 channels count
      SAMPLERATE32BE + // +8 be samplerate up to 192000
      SAMPLES32BE +    // +c be samples up to 1000000000
      ANY16 +          // +10 highpass freq
      "04      |03|05" // +12 version
      "00|08|09|00|00"
      ""_ss
    },
    // AWB, ~6.9k - two variations
    {
      "AWB"_id, "CRI Atom Wave Bank"sv, ".awb",
      "'A'F'S'2"           // +0
      "?{4}"               // +4
      "01-ff 000000"       // +8 le32 subsongs count, assume 1..255
      ""sv,
      PluginType::MULTITRACK
    },
    {
      "CPK"_id, "CRI CPK container"sv, ".awb",
      "'C'P'K' "           // +0
      "?{12}"              // +4
      "'@'U'T'F"           // +10
      ""sv,
      PluginType::MULTITRACK
    },
    // AAX, ~5.3k
    {
      "AAX"_id, "CRI AAX"sv, ".aax",
      "'@'U'T'F"         // +0 sign
      ""sv
    },
    // ACB, ~3.7k
    {
      "ACB"_id, "CRI Atom Cursheet Binary"sv, ".acb",
      "'@'U'T'F"           // +0
      " "sv,              // <- to not to confuse with AAX format
      PluginType::MULTITRACK
    },
    // AIX, ~2.3k
    {
      "AIX"_id, "CRI AIX"sv, ".aix",
      "'A'I'X'F"             // +0
      "????"                 // +4
      "01 00 00 14"          // +8
      "00 00 08 00"          // +c
      ""sv
    },
    // AHX, ~90 requires MPEG_DECODER
    {
      "AHX"_id, "CRI AHX"sv, ".ahx",
      "8000"                 // +0
      "??"                   // +2
      "10|11"                // +4 type
      "00"                   // +5 frame size
      "00"                   // +6 bps
      "01"_ss +              // +7 channels
      SAMPLERATE32BE +       // +8
      SAMPLES32BE +          // +c
      "??"                   // +10
      "06"_ss                // +12
    },

    //Nintendo
    // CSTM, ~30.3k
    {
      "CSTM"_id, "Nintendo CSTM"sv, ".bcstm",
      "'C'S'T'M"        // +0 sign
      "ff fe"           // +4 BOM
      ""sv
    },
    // STRM, ~14.4k
    {
      "STRM"_id, "Nintendo STRM"sv, ".strm",
      "'S'T'R'M"          // +0 sign
      "ff|fe fe|ff 00 01" // +4 flag
      "?{8}"              // +8
      "'H'E'A'D"          // +10
      "50 00 00 00"       // +14 head size
      "00|01|02"          // +18 codec
      "?"                 // +19 loop flag
      "01|02"             // +1a channels count
      "?"_ss +            // +1b
      SAMPLERATE16LE +    // +1c le samplerate from 256hz
      "?{6}"_ss +         // +1e
      SAMPLES32LE         // +24 le samples up to 1000000000
    },
    // RSTM, ~10.9k
    {
      "RSTM"_id, "Nintendo RSTM"sv, ".brstm", // also .brstmspm, but only one example at joshw
      "'R'S'T'M"         // +0 sign
      "feff 00|01 01|00" // +4
      ""sv
    },
    // FSTM, ~10.8k
    {
      "FSTM"_id, "Nintendo FSTM"sv, ".bfstm",
      "'F'S'T'M"        // +0 sign
      "fe|ff ff|fe"     // +4 BOM
      ""sv
    },
    // DSP, ~10.4k, more strict checks
    {
      "DSP"_id, "Nintendo ADPCM"sv, ".dsp",
      "00-10 ???"              // +0 be samples up to 0x10000000
      "00-10 ???"              // +4 be nibbles count up to 0x10000000
      "0000 1f-bb ?"           // +8 be samplerate 8k...48k 1f40..bb80
      "00 00|01"               // +c be loop flag 0/1
      "0000"                   // +e format
      "?{8}"                   // +10 loop start/end offset
      "000000 00|02"           // +18 initial offset 0/2
      "?{32}"                  // +1c
      "0000"                   // +3c gain
      ""sv
    },
    {
      "DSP"_id, "Nintendo ADPCM (LE)"sv, ".adpcm",
      "??? 00-10"              // +0 le samples up to 0x10000000
      "??? 00-10"              // +4 le nibbles count
      "? 1f-bb 0000"           // +8 le samplerate 8k..48k
      "00|01 00"               // +c le loop flag 0/1
      "0000"                   // +e format
      "?{8}"                   // 10 loop start/end offset
      "00|02 000000"           // +18 le initial offset 0/2
      "?{32}"                  // +1c
      "0000"                   // +3c gain
      ""sv
    },
    // CWAV, ~6.9k
    {
      "CWAV"_id, "Nintendo CWAV"sv, ".bcwav",
      "'C'W'A'V"                // +0 signature
      "fffe4000"                // +4
      "00 00-01 00-01 02"
      ""sv
    },
    // ADP, ~3k
    {
      "ADP"_id, "Nintendo ADP (raw)"sv, ".adp",
      // up to 10 frames, upper nibble 0..3, lower 0..c, first byte not zero
      "("
      "01-0c|10-1c|20-2c|30-3c" // +0 != 0
      "00-3c"                   // +1
      "01-0c|10-1c|20-2c|30-3c" // +2 == +0
      "00-3c"                   // +3 == +1
      "?{28}"                   // +4  total x20
      "){10}"
      ""sv
    },
    // BWAV, ~2.9k
    {
      "BWAV"_id, "Nintendo BWAV"sv, ".bwav",
      "'B'W'A'V"                  // +0
      "?{10}"_ss +                // +4
      CHANNELS16LE +              // +e
      "00-01 00"_ss               // +10 codec
    },
    // FWAV, ~2.4k
    {
      "FWAV"_id, "Nintendo FWAV"sv, ".fwav", // also .bfwav,.bfwavnsmbu
      "'F'W'A'V"        // +0 sign
      "fe|ff ff|fe"     // +4 BOM
      ""sv
    },
    // SWAV, ~1.8k
    {
      "SWAV"_id, "Nintendo SWAV"sv, ".swav",
      "'S'W'A'V"       // +0
      "?{12}"          // +4
      "'D'A'T'A"       // +10
      "????"           // +14
      "00|01|02"       // +18
      "?"_ss +         // +19
      SAMPLERATE16LE   // +1a
    },
    // RWAV, ~1.2k
    {
      "RWAV"_id, "Nintendo RWAV"sv, ".rwav",
      "'R'W'A'V"                // +0 signature
      "feff0102"                // +4
      ""sv
    },
    // THP, ~1k
    {
      "THP"_id, "Nintendo THP"sv, ".thp",
      "'T'H'P 00"      // +0
      "0001 00|10 00"  // +4 version
      ""sv
    },
    // OPUS, ~650 init_vgmstream_opus with offset=0
    {
      "OPUS"_id, "Nintendo OPUS"sv, ".opus",
      "01000080"       // +0
      "?{4}"           // +4
      "?"_ss +         // +8 version
      CHANNELS8 +      // +9
      ANY16 +          // +a
      SAMPLERATE32LE   // +c
    },
    // AST, ~610
    {
      "AST"_id, "Nintendo AST"sv, ".ast",
      "'S|'M 'T|'R 'R|'T 'M|'S"          // +0 sign
      "????"                             // +4 rest size
      "00 00|01"                         // +8 be codec
      // other fields are really endian-dependent
      ""sv
    },

    //Sony
    // ADS, ~21.3k
    {
      "ADS"_id, "Sony AudioStream"sv, ".ads", // also ss2
      "'S'S'h'd"               // +0 signature
      "18|20 000000"           // +4 le header size
      "01|02|10 0000 00|80"_ss +// +8 le codec
      SAMPLERATE32LE +         // +c le samplerate
      CHANNELS32LE             // +10 le channels
    },
    // XA, ~16.1k
    {
      "XA"_id, "Sony XA"sv, ".xa",
      "'R'I'F'F"
      "????"
      "'C'D'X'A"
      "'f'm't' "
      ""sv,
      PluginType::MULTITRACK
    },
    {
      "XA"_id, "Sony XA (raw)"sv, ".xa",
      "00ff{10}00"
      ""sv,
      PluginType::MULTITRACK
    },
    // VAG, ~13.6k - also filename-based quirks
    {
      "VAG"_id, "Sony VAG"sv, ".vag",
      // +0 sign
      "'p|'V"
      "'G|'A"
      "'A|'G"
      "'V|'1|'2|'i|'p|'V"
      ""sv
    },
    // XVAG, ~4.6k
    {
      "XVAG"_id, "Sony XVAG"sv, ".xvag",
      "'X'V'A'G"        // +0
      ""sv,
      PluginType::MULTITRACK
    },
    // MSF, ~3.6k
    {
      "MSF"_id, "Sony MultiStream File"sv, ".msf",
      "'M'S'F ?"                  // +0
      "00 00 00 00|01|03-07"_ss + // +4 be codec
      CHANNELS32BE +              // +8
      ANY32 +                     // +c be data size
      // SAMPLERATE32BE|ffffffff     +10
      "00|ff"
      "00-02|ff"
      ""_ss
    },
    // SGX/SGD/SGB, ~70, TODO: fix header+data processing, filename-based, SGH is multifile
    /*{
      "SGX"_id, "Sony SGX"sv, ".sgx",
      "'S'G'X'D"        // +0
      "?{12}"           // +4
      "'W'A'V'E"        // +10
      "?{512}"          // +14 - to ensure minimal size
      // SGH has the same structure, but smaller size, paired with SGB
    },*/
    // RXWS, ~1.2k
    {
      "XWS"_id, "Sony XWS"sv, ".xws",
      "'R'X'W'S"        // +0
      "?{12}"           // +4
      "'F'O'R'M"        // +10
      ""sv,
      PluginType::MULTITRACK
    },
    {
      "RXW"_id, "Sony XWS (bad rips)"sv, ".rxw",
      "'R'X'W'S"        // +0
      "?{12}"           // +4
      "'F'O'R'M"        // +10
      "?{26}"_ss +      // +14
      SAMPLERATE32LE,    // +2e
      PluginType::MULTITRACK
    },
    // MIC, ~560
    {
      "MIC"_id, "Sony MultiStream MIC"sv, ".mic",
      "40000000"_ss +             // +0
      ANY32 +                     // +4
      CHANNELS32LE +              // +8
      SAMPLERATE32LE              // +c
    },
    // SXD, ~900 - same problems as in SGX
    // BNK, ~170
    {
      "BNK"_id, "Sony BNK (LE)"sv, ".bnk",
      "03000000"                     // +0
      "02|03 000000"                 // +4
      ""sv,
      PluginType::MULTITRACK
    },
    {
      "BNK"_id, "Sony BNK (BE)"sv, ".bnk",
      "00000003"                    // +0
      "000000 02|03"                // +4
      ""sv,
      PluginType::MULTITRACK
    },

    //Audiokinetic
    // WEM, ~18.6k
    {
      "WEM"_id, "Audiokinetic Wwise"sv, ".wem",
      "'R'I'F 'F|'X"           // +0 RIFF/RIFX
      "????"
      "'W|'X"                  // +8 WAVE/XWMA
      "'A|'W"
      "'V|'M"
      "'E|'A"
      ""sv
    },
    // BNK, ~160
    {
      "BNK"_id, "Audiokinetic Wwise Soundbank Container"sv, ".bnk",
      "'A|'B"
      "'K|'K"
      "'B|'H"
      "'K|'D"
      ""sv,
      PluginType::MULTITRACK
    },

    //Electronic Arts
    // SCHl (variable), ~13.9k
    {
      "SCHL"_id, "Electronic Arts SCHL"sv, ".",
      "'S 'C|'H"          // +0 sign - SCHl or SH{locale}
      "'H|'E|'F|'G|'D|'I|'S|'E|'M|'R|'J|'J|'P|'B"
      "'l|'N|'R|'E|'E|'T|'P|'S|'X|'U|'A|'P|'L|'R"
      //"????"              // +4 header size
      ""sv
    },
    // SNR, ~4.5k
    {
      "SNR"_id, "Electronic Arts SNR"sv, ".snr",
      // 4 bit version 0/1
      // 4 bit codec 2..c,e
      "02-0c|0e|12-1c|1e"
      // 6 bit channel config (channels - 1)
      // 18 bits samplerate (up to 200k)
      "???"
      // 2 bits type (00..10)
      // 1 bit loop flag
      // 29 bits samples count
      "%00xxxxxx|%01xxxxxx|%10xxxxxx"
      ""sv,
      // Too weak signature, if multifile, may hide another formats
      //PluginType::MULTIFILE  // <- usually additional file name depends on base one
    },
    // SPS, ~2.2k + ~2.4k under MPEG support
    {
      "SPS"_id, "Electronic Arts SPS"sv, ".sps",
      "48 ???"               // +0 header block id + header block size
      // vvvv (0/1) cccc (any) CCCCCC samplerate18...
      "02-0c|0e|12-1c|1e"    // +4 header1 version, codec, channels-1, samplerate
      "???"
      // tt l samples29...
      "%00xxxxxx|%01xxxxxx|%10xxxxxx"
      ""sv
    },
    // SNU, ~900
    /*{ TODO: enable after fix in hanging calculate_eaac_size
      "SNU"_id, "Electronic Arts SNU"sv, ".snu",
      "?{16}"                // +0
      // standard EA core header
      // 4 bit version 0/1
      // 4 bit codec 2..c,e
      "02-0c|0e|12-1c|1e"
      // 6 bit channel config (channels - 1)
      // 18 bits samplerate (up to 200k)
      "???"
      // 2 bits type (00..10)
      // 1 bit loop flag
      // 29 bits samples count
      "%00xxxxxx|%01xxxxxx|%10xxxxxx"
      ""sv
    },*/
    // MPF, ~480 - multitrack+multifile
    /*{
      "MPF"_id, "Electronic Arts MPF"sv, ".mpf",
      "'P|'x"               // +0
      "'F|'D"
      "'D|'F"
      "'x|'P"
      ""sv
    },*/
    // BNK, ~300
    {
      "BNK"_id, "Electronic Arts BNK"sv, ".bnk",
      "'B'N'K 'l|'b"         // +0
      ""sv,
      PluginType::MULTITRACK
    },

    //Apple
    // M4A, 11k
    {
      "M4A"_id, "Apple QuickTime Container"sv, ".m4a",
      "000000?"                   // +0
      "'f't'y'p"                  // +4
      ""sv,
      PluginType::MULTITRACK
    },
    // AIFF, ~2k
    {
      "AIFF"_id, "Apple Audio Interchange File Format"sv, ".aiff",
      "'F'O'R'M"       // +0
      "????"           // +4
      "'A'I'F'F"       // +8
      ""sv
    },
    // AIFC, ~1.3k
    {
      "AIFC"_id, "Apple Compressed Audio Interchange File Format"sv, ".aifc",
      "'F'O'R'M"       // +0
      "????"           // +4
      "'A'I'F'C"       // +8
      ""sv
    },
    // CAF, ~620
    {
      "CAF"_id, "Apple Core Audio Format File"sv, ".caf",
      "'c'a'f'f"       // +0
      "00010000"       // +4
      ""sv
    },

    //Ubisoft
    // singletrack BAO, ~6.1k, multifile
    // atomic with no subtracks
    {
      "BAO"_id, "Ubisoft BAO (atomic)"sv, ".bao",
      "01|02"
      "1b   |1f      |21|22         |23|25         |27|28"
      "01|02|00      |00|00|ff      |00|01         |01|03"
      "00   |08|10|11|0c|0d|15|1b|1e|08|08|0a|19|1d|02|03"
      ""sv,
      PluginType::MULTIFILE
    },
    // RAK, ~3.5k TODO: merge after parallel formats implementation
    {
      "RAK"_id, "Ubisoft RAKI"sv, ".rak",
      "'R'A'K'I"                  // +0
      "?{28}"                     // +4
      "'f'm't' "                  // +20
      ""sv
    },
    {
      "RAK"_id, "Ubisoft RAKI (modified)"sv, ".rak",
      "????"                      // +0
      "'R'A'K'I"                  // +0+4
      "?{28}"                     // +4+4
      "'f'm't' "                  // +20+4
      ""sv
    },
    // LWAV, LyN ~1.7k, Jade ~840
    {
      "LWAV"_id, "Ubisoft RIFF"sv, ".lwav",
      "'R'I'F'F"       // +0
      "????"           // +4
      "'W'A'V'E"       // +8
      ""sv
    },
    {
      "LWAV"_id, "Ubisoft LyN"sv, ".lwav",
      "'L'y'S'E"        // +0
      "?{16}"           // +4
      "'R'I'F'F"        // +14
      "'W'A'V'E"        // +18
      ""sv
    },
    // CKD, ~1k, endian-dependend fields
    {
      "CKD"_id, "Ubisoft CKD"sv, ".ckd",
      "'R'I'F'F"        // +0
      "????"            // +4
      "'W'A'V'E"        // +8
      "?{8}"            // +c
      "00   |01| 02|55|66" // +14 BE+LE
      "02|55|66| 00   |01"
      ""sv
    },

    //Konami
    // BMP, ~4.9k
    {
      "BMP"_id, "Konami BMP"sv, ".bin",
      "'B'M'P 00"_ss +      // +0 signature
      SAMPLES32BE +         // +4 samples count
      "?{8}"                // +8
      "02 ???"_ss +         // +10 channels, always stereo
      SAMPLERATE32BE        // +14 samplerate
    },
    // 2DX9, ~4.7k
    {
      "2DX9"_id, "Konami 2DX9"sv, ".2dx9",
      "'2'D'X'9"            // +0
      "?{20}"               // +4
      "'R'I'F'F"            // +18
      "????"                // +1c
      "'W'A'V'E"            // +20
      "'f'm't' "            // +24
      "?{6}"_ss +           // +28
      CHANNELS16LE +        // +2e
      SAMPLERATE32LE        // +30
    },
    // IFS, ~1.5k
    {
      "IFS"_id, "Konami Arcade Games Container"sv, ".ifs",
      "6cad8f89"        // +0
      "0003"            // +4
      ""sv,
      PluginType::MULTITRACK
    },
    // SVAG, ~1k
    {
      "SVAG"_id, "Konami SVAG"sv, ".svag",
      "'S'v'a'g"             // +0
      "?{4}"_ss +            // +4
      SAMPLERATE32LE +       // +8
      CHANNELS16LE           // +c
    },
    // SD9, ~470
    {
      "SD9"_id, "Konami SD9"sv, ".sd9",
      "'S'D'9 00"            // +0
      "?{28}"                // +4
      "'R'I'F'F"             // +20
      "????"                 // +24
      "'W'A'V'E"             // +28
      "'f'm't' "             // +2c
      ""sv
    },
    // KCEY, ~200
    {
      "KCEY"_id, "Konami KCE Yokohama"sv, ".kcey",
      "'K'C'E'Y"_ss +        // +0
      ANY32 +                // +4
      CHANNELS32BE +         // +8
      SAMPLES32BE            // +c
    },
    // MSF, ~140
    {
      "MSF"_id, "Konami MSF"sv, ".msf",
      "'M'S'F'C"             // +0
      "00000001"_ss +        // +4 be codec
      CHANNELS32BE +         // +8
      SAMPLERATE32BE         // +c
    },
    // DSP, ~370 - too weak structure

    //Koei
    // KOVS/KVS, ~9.8k
    {
      "KVS"_id, "Koei Tecmo KVS"sv, ".kvs",
      "'K'O'V'S"
      ""sv
    },
    // KTSS, ~2.5k
    {
      "KTSS"_id, "Koei Tecmo KTSS"sv, ".ktss",
      "'K'T'S'S"                  // +0
      "?{28}"                     // +4
      "02|09 ?"                   // +20 - codec
      "?{6}"                      // +22 - version etc
      "01-40"_ss +                // +28 - tracks
      CHANNELS8 +                 // +29
      ANY16 +                     // +2a
      SAMPLERATE32LE              // +2c
    },
    // MIC, ~2.3k
    {
      "MIC"_id, "Koei Tecmo MIC"sv, ".mic",
      "00 08 00 00"_ss +     // +0 le start offset = 0x800
      SAMPLERATE32LE +       // +4
      CHANNELS32LE           // +8
    },
    // DSP, ~460
    {
      "DSP"_id, "Koei Tecmo WiiBGM"sv, ".dsp",
      "'W'i'i'B'G'M 00 00"_ss +   // +0
      ANY32 +                     // +8
      ANY32 +                     // +c
      SAMPLES32BE +               // +10
      "?{15}"_ss +                // +14
      CHANNELS8 +                 // +23
      ANY16 +                     // +24
      SAMPLERATE16BE              // +26
    },

    //Square Enix
    // SAB, ~2.7k
    {
      "SAB"_id, "Square Enix SAB"sv, ".sab",
      "'s'a'b'f"       // +0
      ""sv,
      PluginType::MULTITRACK
    },
    // AKB, ~2.1k
    {
      "AKB"_id, "Square Enix AKB"sv, ".akb",
      "'A'K'B' "        // +0
      "?{8}"            // +4
      "02|05|06"_ss +   // +c codec
      CHANNELS8 +       // +d
      SAMPLERATE16LE +  // +e
      SAMPLES32LE       // +10
    },
    // MAB, ~1.7k
    {
      "MAB"_id, "Square Enix MAB"sv, ".mab",
      "'m'a'b'f"       // +0
      ""sv,
      PluginType::MULTITRACK
    },
    // AKB, ~1k
    {
      "AKB"_id, "Square Enix AKB2"sv, ".akb",
      "'A'K'B'2"        // +0
      "?{8}"            // +4
      "01|02"           // +c table count
      ""sv,
      PluginType::MULTITRACK
    },
    // SCD, ~980
    {
      "SCD"_id, "Square Enix SCD"sv, ".scd",
      "'S'E'D'B'S'S'C'F"   // +0
      ""sv,
      PluginType::MULTITRACK
    },
    // BGW, ~330
    {
      "BGW"_id, "Square Enix BGW"sv, ".bgw",
      "'B'G'M'S't'r'e'a'm 000000" // +0
      "00|03 000000"              // +c codec
      ""sv
    },

    //Namco
    // IDSP, ~2.1k
    // init_vgmstream_nub_idsp
    {
      "IDSP"_id, "Namco IDSP (NUB container)"sv, ".idsp",
      "'i'd's'p"          // +0
      "?{184}"            // +4
      "'I'D'S'P"          // +0+188
      "????"              // +4+188
      "00 00 00 01-08"    // +8+188, channels
      ""sv
    },
    // NPS, ~770
    {
      "NPS"_id, "Namco NPSF"sv, ".nps",
      "'N'P'S'F"               // +0 signature
      "?{8}"_ss +              // +4
      CHANNELS32LE +           // +c le channels
      "?{8}"_ss +              // +10
      SAMPLERATE32LE           // +18
    },
    // NAAC, ~370
    {
      "NAAC"_id, "Namco AAC"sv, ".naac",
      "'A'A'C' "          // +0
      "01000000"_ss +     // +4
      ANY32 +             // +8
      SAMPLERATE32LE +    // +c
      SAMPLES32LE         // +10
    },
    // CSTR, ~300
    {
      "DSP"_id, "Namco CSTR"sv, ".dsp",
      "'C's't'r"               // +0 signature
      "?{23}"_ss +             // +4
      CHANNELS8 +              // +1b channels
      "????"_ss +              // +1c
      SAMPLES32BE +            // +20 be samples up to 1000000000
      "????"_ss +              // +24
      SAMPLERATE32BE           // +28 be samplerate up to 192000
    },
    // DSB, ~110, requires G7221
    /*{
      "DSB"_id, "Namco DSB"sv, ".dsb",
      "'D'S'S'B"               // +0
      "?{60}"                  // +4
      "'D'S'S'T"               // +40
    },*/
    // NUB,NUB2,NUS3 a lot subformats
    {
      "NUS3"_id, "Namco nuSound container v3"sv, ".nus3bank",
      "'N'U'S'3"          // +0
      "????"              // +4
      "'B'A'N'K'T'O'C' "  // +8
      ""sv,
      PluginType::MULTITRACK
    },
    {
      "NUB"_id, "Namco nuSound container"sv, ".nub",
      // +0
      "00   |01"
      "02   |02"
      "00|01|01"
      "00   |00"
      // +4
      "00000000"
      ""sv,
      PluginType::MULTITRACK
    },

    //Capcom
    // MCA, ~2.1k
    {
      "MCA"_id, "Capcom MCA"sv, ".mca",
      "'M'A'D'P"        // +0 sign
      "????"_ss +
      CHANNELS8 +       // +8 channels count
      "???"_ss +
      SAMPLES32LE +     // +c le samples up to 1000000000
      SAMPLERATE16LE    // +10 le samplerate from 256hz
    },
    // AUS, ~1.1k
    {
      "AUS"_id, "Capcom AUS"sv, ".aus",
      "'A'U'S' "        // +0
      "?{4}"_ss +       // +4
      SAMPLES32LE +     // +8
      CHANNELS32LE +    // +c
      SAMPLERATE32LE    // +10
    },
    // LOPUS, ~500
    {
      "LOPUS"_id, "Capcom OPUS container"sv, ".lopus",
      SAMPLES32LE +     // +0
      "01|02|06 000000" // +4
      "?{16}"           // +8
      "0000"_ss         // +18
    },
    // DSPW, ~310
    {
      "DSPW"_id, "Capcom DSPW"sv, ".dspw",
      "'D'S'P'W"        // +0
      ""sv
    },
    // DVI, ~100
    {
      "DVI"_id, "Capcom IDVI"sv, ".dvi",
      "'I'D'V'I"_ss +   // +0
      CHANNELS32LE +    // +4
      SAMPLERATE32LE    // +8
    },

    //Crystal Dynamics
    // MUL, ~1.5k
    {
      "MUL"_id, "Crystal Dynamics MUL"sv, ".mul",
      // +0 samplerate4 8000..48000 (#1f40..#bb80)
      // +c channels4 1..8
      "0000 1f-bb ?" // +0
      "?{8}"         // +4
      "000000 01-08" // +c
      ""sv
    },
    {
      "MUL"_id, "Crystal Dynamics MUL (BE)"sv, ".mul",
      // +0 samplerate4 8000..48000 (#1f40..#bb80)
      // +c channels4 1..8
      "? 1f-bb 0000" // +0
      "?{8}"         // +4
      "01-08 000000" // +c
      ""sv
    },

    //Microsoft
    // XMA, ~48k
    {
      "XMA"_id, "Microsoft XMA"sv, ".xma",
      "'R'I'F'F"         // +0
      ""sv
    },
    // XWB, ~8.5k
    {
      "XWB"_id, "Microsoft XACT Wave Bank"sv, ".xwb",
      // +0 sign
      "'W|'D"
      "'B|'N"
      "'N|'B"
      "'D|'W"
      ""sv,
      PluginType::MULTITRACK
    },
    // XWM, ~900
    {
      "XWM"_id, "Microsoft XWMA"sv, ".xwm",
      "'R'I'F'F"          // +0
      "????"              // +4
      "'X'W'M'A"          // +8
      ""sv,
      PluginType::MULTITRACK
    },
    // XNB, ~590
    {
      "XNB"_id, "Microsoft XNA Game Studio XNB"sv, ".xnb",
      "'X'N'B ?"          // +0
      "04|05"             // +4
      ""sv,
      PluginType::MULTIFILE // also multitrack, but no samples
    },

    //Hudson
    // PDT, ~440
    {
      "PDT"_id, "Hudson Splitted Stream container"sv, ".pdt",
      "'P'D'T' "           // +0
      "'D'S'P' "           // +4
      "'H'E'A'D"           // +8
      "'E'R"               // +c
      "01|02 00"           // +e channels
      ""sv
    },
    // PDT, ~440
    {
      "PDT"_id, "Hudson Stream Container"sv, ".pdt",
      "0001 ??"            // +0 be 1
      "000000 02|04"       // +4 be 2/4
      "00007d00"           // +8 be 0x7d00
      "000000 02|04"       // +c be 2/04
      ""sv,
      PluginType::MULTITRACK
    },
    // H4M, ~170
    {
      "H4M"_id, "Hudson HVQM4"sv, ".h4m",
      "'H'V'Q'M'4' '1'."   // +0
      "'3|'5 000000"       // +8
      "?{4}"               // +c
      "00000044"           // +10
      ""sv,
      PluginType::MULTITRACK
    },

    //RAD Game Tools
    // BIK, ~19.9k
    {
      "BIK"_id, "RAD Game Tools Bink"sv, ".bik",
      // +0 signature
      "'B|'K"
      "'I|'B"
      "'K|'2"
      "?"
      "????"         // +4 filesize
      "?? 00-0f 00"  // +8 le frames up to 0x100000
      "?{28}"        // +c
      "01-ff 000000" // +28 le total subsongs 1..255
      ""sv,
      PluginType::MULTITRACK
    },
    // SMK, ~190
    {
      "SMK"_id, "RAD Game Tools SMACKER"sv, ".smk",
      "'S'M'K '2|'4"   // +0
      "?{8}"           // +4
      "?? 00-0f 00"    // +c le frames up to 0x100000
      ""sv
      //PluginType::MULTITRACK <- really no multitrack samples...
    },

    //Radical
    // RSD, ~4.4k
    {
      "RSD"_id, "Radical RSD"sv, ".rsd",
      "'R'S'D"           // +0
      "'2|'3|'4|'6"      // +3 version
      // +4 codec
      "'P   |'V|'X|'G|'W|'R|'O|'W|'A|'X"
      "'C   |'A|'A         |'O|'M|'T|'M"
      "'M   |'G|'D         |'G|'A|'3|'A"
      "' |'B|' |'P         |'V|' |'+|' "
      ""_ss +
      CHANNELS32LE +     // +8
      ANY32 +            // +c
      SAMPLERATE32LE     // +10
    },
    // P3D, ~1.5k
    {
      "P3D"_id, "Radical P3D"sv, ".p3d",
      "'P|ff"        // +0
      "'3|'D"
      "'D|'3"
      "ff|'P"
      "0c|00"        // +4
      "0000"
      "00|0c"
      "?{4}"         // +8
      "fe|00"        // +c
      "0000"
      "00|fe"
      "?{8}"         // +10
      "0a|00"        // +18
      "0000"
      "00|0a"
      ""sv
    },

    //Retro Studios
    // RFRM, ~200
    {
      "RFRM"_id, "Retro Studios RFRM"sv, ".csmp",
      "'R'F'R'M"               // +0
      "?{16}"                  // +4
      "'C'S'M'P"               // +14
      ""sv
    },
    // DSP, ~150
    {
      "DSP"_id, "Retro Studios RS03"sv, ".dsp",
      "'R'S 00 03"             // +0 signature
      "00 00 00 01|02"_ss +    // +4 be channels count
      SAMPLES32BE +            // +8 be samples up to 1000000000
      SAMPLERATE32BE           // +c be samplerate up to 192000
    },

    //Banpresto
    // WMSF, ~600
    {
      "WMSF"_id, "Banpresto MSF wrapper"sv, ".msf",
      "'W'M'S'F"            // +0
    },
    // 2MSF, ~290
    {
      "2MSF"_id, "Banpresto RIFF wrapper"sv, ".at9",
      "'2'M'S'F"            // +0
    },

    //Bizzare Creations
    // BAF, ~290
    {
      "BAF"_id, "Bizzare Creations bank file"sv, ".baf",
      "'B'A'N'K"             // +0
      "?{4}"                 // +4
      "00|03|04|05"          // +8 be/le 3/4/5
      "0000"
      "00|03|04|05"
      ""sv,
      PluginType::MULTITRACK
    },
    {
      "BAF"_id, "Bizzare Creations band file (bad rips)"sv, ".baf",
      "'W'A'V'E"             // +0
      "0000004c"             // +4
      // "DATA" @ 4c
      ""sv,
      PluginType::MULTITRACK
    },

    //Other
    // FSB, v1 - 30, v2 - no, v3 - 3.9k, v4 - 19.4k, v5 - 203.8k, some of them are already supported
    {
      "FSB"_id, "FMOD Sample Bank (v1-v5)"sv, ".fsb",
      "'F'S'B '1-'5"               // +0 sign
      ""sv,
      PluginType::MULTITRACK
    },
    // GENH, ~15.3k
    {
      "GENH"_id, "Generic Header"sv, ".genh",
      "'G'E'N'H"_ss +               // +0 sign
      CHANNELS32LE +                // +4 channels
      ANY32 +                       // +8 interleave
      SAMPLERATE32LE +              // +c samplerate
      ANY32 +                       // +10 loop start sample
      ANY32 +                       // +14 loop end sample
      "00-1c 000000"                // +18 codec 0..28
      //"????"                        // +1c start offset
      //"????"                        // +20 header size
      ""_ss
    },
    // MIB, ~5.7k - way too weak, also filename-dependent
    /*{
      "MIB"_id, "Headerless PS-ADPCM"sv, ".mib",
      // Px F <any> (16 total) P=0..5 F=0..7 up to 1k total
      "("
      "00-5f"
      "00-07"
      "?{14}"
      "){16}"
    },*/
    // IDSP, ~3.6k= ~2.1k Namco, ~720 Next level, ~720 Traveller and other
    {
      "IDSP"_id, "Various IDSP"sv, ".idsp",
      "'I'D'S'P"          // +0
      ""sv
    },
    // ACM, ~3.1k
    {
      "ACM"_id, "InterPlay ACM"sv, ".acm",
      "97 28 03 01"               // +0
      ""sv
    },
    // OPUS, ~3k
    {
      "OPUS"_id, "Various OPUS containers"sv, ".opus",
      "'O'P'U'S"    // +0
      ""sv
    },
    // XA, ~2.8k conflicts with 'Nintendo ADPCM (LE)'
    {
      "XA"_id, "Maxis XA"sv, ".xa",
      "'X'A"                      // +0
      "?{6}"                      // +2
      "01 00"_ss +                // +8 format tag
      CHANNELS16LE +              // +a
      SAMPLERATE32LE              // +c
    },
    // VPK, ~2.7k
    {
      "VPK"_id, "SCE America VPK"sv, ".vpk",
      "' 'K'P'V"                  // +0
      "?{12}"_ss +                // +4
      SAMPLERATE32LE +            // +10
      CHANNELS32LE                // +14
    },
    // MUSX, ~2.3k
    {
      "MUSX"_id, "Eurocom MUSX"sv, ".musx",
      "'M'U'S'X"       // +0
      "????"           // +4
      "01|c9|04-06|0a" // +8 version
      ""sv,
      PluginType::MULTITRACK
    },
    // RPGMVO, ~1.7k
    {
      "RPGMVO"_id, "Encrypted OGG Vorbis"sv, ".rpgmvo",
      "'R'P'G'M'V 000000" // +0
      ""sv
    },
    {
      "LOGG"_id, "Encrypted OGG Vorbis"sv, ".logg",
      "2c|4c|04|4f"
      "44|32|86|75"
      "44|53|86|6f"
      "30|44|c5|71"
      ""sv
    },
    {
      "LOGG"_id, "Corrupted OGG Vorbis"sv, ".logg",
      // TODO: support negation in format notation
      "00-4e|50-ff"  // !'O  +0
      "00-66|68-ff"  // !'g
      "00-66|68-ff"  // !'g
      "00-52|54-ff"  // !'S
      "?{54}"
      "'O'g'g'S"     // +3a
      ""sv
    },
    // OPUS, ~1.7k
    {
      "OPUS"_id, "OGG Opus"sv, ".opus",
      "'O'g'g'S"     // +0
      "?{24}"        // +4
      "'O'p'u's'H'e'a'd" // +1c assume one-lace first page
      ""sv
    },
    // RWS, ~1.7k
    {
      "RWS"_id, "RenderWare Stream"sv, ".rws",
      "0d080000"       // +0
      "????"           // +4 file size
      "????"           // +8
      "0e080000"       // +c
      ""sv,
      PluginType::MULTITRACK
    },
    // XWC, ~1400
    {
      "XWC"_id, "Starbreeze XWC"sv, ".xwc",
      "00"
      "03|04"
      "00"
      "00"
      "00"
      "90"
      "00"
      "00"
      ""sv
    },
    // AUD (new), ~1.2k
    {
      "AUD"_id, "Westwood Studios AUD"sv, ".aud",
      SAMPLERATE16LE + // +0
      "?{8}"           // +2
      "%xxxxxxx0"      // +a only mono supported by libvgmstream
      "01|63"          // +b format
      "?{4}"           // +c
      "afde0000"       // +10
      ""_ss
    },
    // BSND, ~1.2k - filename-based
    /*{
      "BSND"_id, "id Tech 5 audio"sv, ".bsnd",
      "'b's'n'f 00"    // +0
      "00000100"       // +5
    },*/
    // HPS, ~1k
    {
      "HPS"_id, "HAL Labs HALPST"sv, ".hps",
      "' 'H'A'L'P'S'T 00"_ss + // +0 signature
      ANY32 +                  // +8
      CHANNELS32BE             // +c be channels count
    },
    {
      "BANK"_id, "FSB5 FEV"sv, ".bank",
      "'R'I'F'F ???? 'F'E'V' "_ss
    },
    // MUSC, ~850
    {
      "MUSC"_id, "Krome MUSC"sv, ".musc",
      "'M'U'S'C"_ss +          // +0
      ANY16 +                  // +4
      SAMPLERATE16LE           // +6
    },
    // SVAG, ~840
    {
      "SVAG"_id, "SNK SVAG"sv, ".svag",
      "'V'A'G'm"_ss +         // +0
      ANY32 +                 // +4
      SAMPLERATE32LE +        // +8
      CHANNELS32LE            // +c
    },
    // AWC, ~830
    {
      "AWC"_id, "Rockstar AWC"sv, ".awc",
      "'A|'T"        // +0
      "'D|'A"
      "'A|'D"
      "'T|'A"
      ""sv,
      PluginType::MULTITRACK
    },
    // ZSS, ~800
    {
      "ZSS"_id, "Z-Axis ZSND"sv, ".zss",
      "'Z'S'N'D"          // +0
      // +4 codec
      "'P|'X|'P|'G"
      "'C|'B|'S|'C"
      "' |'O|'2|'U"
      "' |'X|' |'B"
      ""sv,
      PluginType::MULTITRACK
    },
    // SEB, ~740
    {
      "SEB"_id, "Game Arts SEB"sv, ".seb",
      "01|02 000000"_ss +     // +0 channels count
      SAMPLERATE32LE          // +4
    },
    // SEG, ~700
    {
      "SEG"_id, "Stormfront SEG"sv, ".seg",
      "'s'e'g 00"             // +0
      // +4
      "'p|'x|'w|'p|'x"
      "'s|'b|'i|'c|'b"
      "'2|'x|'i|'_|'3"
      "00"
      ""sv
    },
    // WAVE, ~660
    {
      "WAVE"_id, "EngineBlack WAVE"sv, ".wave",
      "e5|fe"                    // +0
      "b7|ec"
      "ec|b7"
      "fe|e5"
      "00|01 ???"                // +4 version
      ""sv
    },
    // IMUSE, ~650
    {
      "IMUS"_id, "LucasArts iMUSE"sv, ".imc",
      "'C|'M"                    // +0
      "'O|'C"
      "'M"
      "'P"
      ""sv
    },
    // VGS, ~650
    {
      "VGS"_id, "Princess Soft VGS"sv, ".vgs",
      "'V'G'S 00"             // +0
      "?{12}"_ss +            // +4
      SAMPLERATE32BE          // +c
    },
    // SMP, ~640
    {
      "SMP"_id, "Infernal Engine SMP"sv, ".smp",
      "05|06|07|08 000000"    // +0 version
      "?{16}"                 // +4
      "00000000"_ss +         // +14
      SAMPLES32LE +           // +18
      ANY32 +                 // +1c
      ANY32 +                 // +20
      "01|02|04|06|07 000000" // +24
      ""_ss
    },
    // AAC, ~600
    {
      "AAC"_id, "tri-Ace ASKA engine"sv, ".aac",
      "'A|' "               // +0
      "'A|'C"
      "'C|'A"
      "' |'A"
      // v1 "DIR "@0x30
      // v2 no stable parts
      // v3 no stable parts
      ""sv,
      PluginType::MULTITRACK
    },
    // NWA, ~540
    {
      "NWA"_id, "VisualArt's NWA"sv, ".nwa",
      "01|02 00 ??"_ss +    // +0 channels count
      SAMPLERATE32LE +      // +4
      "?{20}"_ss +          // +8
      SAMPLES32LE           // +1c - really channels multiply
    },
    // STER, ~520
    {
      "STER"_id, "Alchemy STER"sv, ".ster",
      "'S'T'E'R"         // +0
      "?{12}"_ss +       // +4
      SAMPLERATE32BE     // +10
    },
    // NXA, ~510
    {
      "NXA"_id, "Entergram NXA"sv, ".nxa",
      "'N'X'A'1"             // +0
      "01|02 000000"         // +4 type
      ""sv
    },
    // STR, ~500
    {
      "STR"_id, "Sega Stream Asset Builder STR"sv, ".str",
      ANY32 +                 // +0
      SAMPLERATE32LE +        // +4
      "04|10 000000"          // +8
      "?{201}"                // +c
      "'S'e'g'a"              // +d5
      ""_ss
    },
    // IKM, ~470
    {
      "IKM"_id, "MiCROViSiON container"sv, ".ikm",
      "'I'K'M 00"             // +0
      "?{28}"                 // +4
      "00|01|03 000000"       // +20
      ""sv
    },
    // NXOPUS, ~400
    {
      "NSOPUS"_id, "Nihon Falcom FDK"sv, ".nxopus",
      "'f'o'x'n"                   // +0
      "?"_ss + CHANNELS8 + ANY16 + // +4
      SAMPLERATE32LE +             // +8
      "?{20}"_ss +                 // +c
      SAMPLES32LE                  // +20
    },
    // SPSD, ~360
    {
      "SPSD"_id, "Naomi SPSD"sv, ".spsd",
      "'S'P'S'D"              // +0
      "01|00 010004"          // +4
      "00|01|03"              // +8 codec
      "?"                     // +9 flags
      "00|0d|ff 00"           // +a le16 index
      "?{30}"_ss +            // +c
      SAMPLERATE16LE          // +2a
    },
    // HWAS, ~300
    {
      "HWAS"_id, "Vicarious Visions HWAS"sv, ".hwas",
      "'s'a'w'h"_ss +          // +0
      ANY32 +                  // +4 usually 0x2000, 0x4000, 0x8000, but for any...
      SAMPLERATE32LE +         // +8
      "01000000"_ss            // +c channels
    },
    // VXN, ~300
    {
      "VXN"_id, "Gameloft VXN"sv, ".vxn",
      "'V'o'x'N"            // +0
      ""sv,
      PluginType::MULTITRACK
    },
    // GCA, ~260
    {
      "GCA"_id, "Terminal Reality GCA"sv, ".gca",
      "'G'C'A'1"               // +0
      "?{38}"_ss +             // +4
      SAMPLERATE32BE           // +2a
    },
    // STM, ~250
    {
      "STM"_id, "Intelligent Systems STM"sv, ".stm",
      "0200"_ss +      // +0
      SAMPLERATE16BE + // +2
      CHANNELS32BE
    },
    // ISWS, ~250
    {
      "ISWS"_id, "Sumo Digital iSWS"sv, ".isws",
      "'i'S'W'S"        // +0
    },
    // XWV, ~250
    {
      "XWV"_id, "feelplus XWAV"sv, ".xwv",
      // +0
      "'V|'X"
      "'A|'W"
      "'W|'A"
      "'X|'V"
    },
    // ATX, ~240, filename-based
    /*{
      "ATX"_id, "Media Vision ATX"sv, ".atx",
      "'A'P'A'3"        // +0
      "?{24}"           // +4
      "0000"            // +1c
      "01|02 00"        // +1e
    },*/
    // PONA, ~230
    {
      "PONA"_id, "Policenauts BGM"sv, ".pona",
      // be 0x13020000(3do)/0x00000800(psx)
      "13|00"
      "02|00"
      "00|08"
      "00|00"
      ""sv
    },
    // SADL, ~230
    {
      "SAD"_id, "Procyon Studio SADL"sv, ".sad",
      "'s'a'd'l"                      // +0
      "?{47}"                         // +4
      "%0000xxxx|%0111xxxx|%1011xxxx" // +33 flags
      ""sv
    },
    // ADS, ~220
    {
      "ADS"_id, "Midway ADS"sv, ".ads",
      "'d'h'S'S"               // +0
      "?{4}"                   // +4
      "00000020|21"_ss +       // +8 codec
      SAMPLERATE32BE +         // +c
      "00000001|02"            // +10 channels
      "?{12}"                  // +14
      "'d'b'S'S"               // +20
      ""_ss
    },
    // STR, ~190
    {
      "STR"_id, "3DO STR"sv, ".str",
      "'C|'S|'S"
      "'T|'N|'H"
      "'R|'D|'D"
      "'L|'S|'R"
      ""sv
    },
    // CKS, ~180
    {
      "CKS"_id, "Cricket Audio Stream"sv, ".cks",
      "'c'k'm'k"               // +0
      "?{4}"                   // +4
      "00000000"               // +8 type
      "02000000"               // +c le version
      "00|01|02"_ss +          // +10 codec
      CHANNELS8 +              // +11
      SAMPLERATE16LE           // +12
    },
    // CAF, ~170
    {
      "CAF"_id, "tri-Crescendo CAF"sv, ".caf",
      "'C'A'F' "               // +0
      ""sv
    },
    // ISD, ~170
    {
      "ISD"_id, "Inti Creates OGG Vorbis"sv, ".isd",
      "af|0f|0f"
      "67|e7|a7"
      "87|87|47"
      "53|d3|53"
      ""sv
    },
    // YDSP, ~170
    {
      "YDSP"_id, "Yuke DSP"sv, ".ydsp",
      "'Y'D'S'P"_ss +          // +0
      ANY32 +                  // +4
      ANY32 +                  // +8
      SAMPLERATE32BE +         // +c
      CHANNELS16BE             // +10
    },
    // RSD, ~150, decoded only
    {
      "RSD"_id, "RedSparc RSD"sv, ".rsd",
      "'R'e'd'S'p'a'r'k"      // +0
      "?{52}"_ss +            // +8
      SAMPLERATE32LE +        // +3c
      SAMPLES32LE +           // +40
      "?{10}"_ss +            // +44
      CHANNELS8               // +4e
    },
    // ADP, ~145
    {
      "ADP"_id, "Nex Entertainment NXAP"sv, ".adp",
      "'N'X'A'P"              // +0
      "?{8}"_ss +             // +4
      CHANNELS32LE +          // +c
      SAMPLERATE32LE +        // +10
      "40000000"              // +14
      "40000000"              // +18
      ""_ss
    },
    // ADP, ~120
    {
      "ADP"_id, "Xilam DERF"sv, ".adp",
      "'D'E'R'F"              // +0
      "01|02 000000"          // +4 le channels, up to 2
      ""sv
    },
    // SDF, ~45
    {
      "SDF"_id, "Beyond Reality SDF"sv, ".sdf",
      "'S'D'F 00"                     // +0
      "03000000"                      // +4
      ""sv
    },
    // ZSD, ~40
    {
      "ZSD"_id, "ZSD container"sv, ".zsd",
      "'Z'S'D 00"                     // +0
      "?{12}"_ss +                    // +4
      SAMPLERATE32LE +                // +10
      ANY32 +                         // +14
      SAMPLES32LE                     // +18, always mono
    },
    // MTX, ~? - should be decoder...
    {
      "MTX"_id, "Matric Software container"sv, ".bin",
      "'F|'m"                  // +0
      "'F|'t"
      "'D|'x"
      "'L|'s"
      ""sv
    },

    //  FFMPEG-based
    // APE, ~20k
    {
      "APE"_id, "Monkey's Audio"sv, ".ape",
      "'M'A'C' "            // +0
      ""sv
    },
    // ASF, ~10k - TODO: multitrack
    {
      "ASF"_id, "Windows Media Audio"sv, ".asf",
      "30 26 B2 75 8E 66 CF 11 A6 D9 00 AA 00 62 CE 6C" // +0 asf guid
      ""sv,
      PluginType::MULTITRACK
    },
    // TAK, ~7.8k
    {
      "TAK"_id, "Tom's lossless Audio Kompressor"sv, ".tak",
      "'t'B'a'K"      // +0
      ""sv
    },
    // AAC, ~4k
    {
      "AAC"_id, "Advanced Audio Coding (raw ADTS)"sv, ".aac",
      // https://wiki.multimedia.cx/index.php/ADTS
      "%11111111 %1111x00x" // +0 w & 0xfff6 == 0xfff0
      ""sv
    },
    // OMA, ~2.5k
    {
      "OMA"_id, "Sony OpenMG audio"sv, ".oma",
      "'e'a'3"
      ""sv
    },
    // MPC, ~2k
    {
      "MPC"_id, "Musepack"sv, ".mpc",
      "'M"
      "'P"
      "'+   |'C"
      "07|17|'K"
      ""sv
    },
    // AC3, ~650
    {
      "AC3"_id, "Dolby AC-3"sv, ".ac3",
      "0b77"          // +0 - also little endian?
      "??"            // +2 crc
      // see ff_ac3_parse_header
      // samplerate2 10/01/00
      // framesize6 0..37
      "00-25|40-65|80-a5"
      // bitstream id5 0..10 (normal ac3) mode3 any
      // %00000xxx-%01010xxx
      "00-57"
      ""sv
    },

    //Unsupported
    // VAS, ~1.1k - no signatures, too few container versions
    // UE4 switch audio, ~1.1k - no signatures
    // STR+WAV, ~1k - TODO: fix libvgmstream to process sth first
    // SSM, ~120 no signatures
    // ISH+ISD, ~90 - after processing header first
    // AFC/STX, ~90 - no signatures
    // RAW, ~800 - no signature
    // TYDSP, ~310 - no signature
    // TXTH, ~22k, not supported by model for now
    // Ubisoft SBx, ~5.4k - multitrack!!! and possible multifile, extension-based detection
    // Headerless PS-ADPCM, ~5.8k - extension-based detection
    // DTK - Nintendo ADP raw header, ~3k - no stable signature
    // ACX, 1 - no signature
    // RRDS, ~25, no signatures
    // S14/SSS, ~210 total - no signature, relies on extension
    // WAVM, ~640 - no signatures
    // SPT+SPD, ~640 - no signatures, multifile
    // PS2 .int raw, ~810 no signatures
    // HXD, ~850 - weak signature
    // MIB+MIH, ~1.7k - multifile, no signatures
    // IVAUD, ~1.5k almost no signatures, only GTA series
    // DSP, ~370 (Konami DSP) - no signature
    // RRDS, ~25 - weak signature
    // STRM (Final Fantasy Tactics, 2sf), ~16
    // OSB+DAT, ~79 - after processing header first
    // ASD, ~16 - weak signature
    // SPT+SPD, ~700 - after processing header first
    // TYDSP, ~300 - no signature
    // MUS, ~150 - Wii, weak signature
    // GCM, ~300 - double DSP, weak signature
    // WVS, ~200 - Swingin' Ape, weak signature
    // BNH+HXG, ~170 - weak signature
    // MPF, ~140 - mulifile+multitrack
    // SSM, ~120 - weak signature
    // NWA, ~850 - weak signature
    // ADP, Quantic Dream ~145 - weak signature
    // GSP+GSB, ~160 - after processing header first
    // MOGG, ~320 - too weak
    // ZWDSP, ~230 - too weak
    // BNS, ~130 - TODO
    // RWSD, ~120 - filename-based
    // VID, ~190 - two endians
    // WAVM, ~610 - no signature
    // XMD, ~310 - no signature for v1, TODO v2
  };
  // clang-format on
}  // namespace Module::VGMStream

namespace ZXTune
{
  void RegisterVGMStreamPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    const uint_t CAPS = Capabilities::Module::Type::STREAM | Capabilities::Module::Device::DAC;
    std::set<StringView> formats;
    for (const auto& desc : Module::VGMStream::PLUGINS)
    {
      Require(formats.insert(desc.Format).second);
      using namespace Module::VGMStream;
      if (desc.Type == PluginType::MULTITRACK)
      {
        auto decoder = MakePtr<MultiTrack::Decoder>(desc);
        auto factory = MakePtr<MultiTrack::Factory>(desc);
        {
          auto plugin = CreatePlayerPlugin(desc.Id, CAPS, decoder, factory);
          players.RegisterPlugin(std::move(plugin));
        }
        {
          auto plugin = CreateArchivePlugin(desc.Id, std::move(decoder), std::move(factory));
          archives.RegisterPlugin(std::move(plugin));
        }
      }
      else
      {
        const auto trait = desc.Type == PluginType::MULTIFILE ? Capabilities::Module::Traits::MULTIFILE : 0;
        auto decoder = MakePtr<SingleTrack::Decoder>(desc);
        auto factory = MakePtr<SingleTrack::Factory>(desc);
        auto plugin = CreatePlayerPlugin(desc.Id, CAPS | trait, std::move(decoder), std::move(factory));
        players.RegisterPlugin(std::move(plugin));
      }
    }
  }
}  // namespace ZXTune
