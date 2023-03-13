/**
 *
 * @file
 *
 * @brief  vgmstream-based formats support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "core/plugins/players/plugin.h"
// common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
#include <static_string.h>
// library includes
#include <binary/container_base.h>
#include <binary/crc.h>
#include <binary/format_factories.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/container.h>
#include <formats/multitrack.h>
#include <math/numeric.h>
#include <module/additional_files.h>
#include <module/attributes.h>
#include <module/players/duration.h>
#include <module/players/platforms.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <sound/resampler.h>
// 3rdparty includes
extern "C"
{
// clang-format off
#include <3rdparty/vgmstream/config.h>
#include <3rdparty/vgmstream/src/vgmstream.h>
#include <3rdparty/vgmstream/src/plugins.h>
  // clang-format on
}
// std includes
#include <algorithm>
#include <array>
#include <map>
#include <set>

namespace Module::VGMStream
{
  const Debug::Stream Dbg("Core::VGMStream");

  class Vfs
  {
  public:
    using Ptr = std::shared_ptr<Vfs>;

    Vfs(StringView rootName, Binary::Data::Ptr rootData)
    {
      Content.emplace_back(rootName.to_string(), std::move(rootData));
    }

    const String& Root() const
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
      for (const auto& entry : Content)
      {
        if (!entry.second)
        {
          return true;
        }
      }
      return false;
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
    explicit MemoryStream(Vfs::Ptr vfs)
      : MemoryStream(vfs, vfs->Root(), vfs->Request(vfs->Root()))
    {}

  private:
    MemoryStream(Vfs::Ptr vfs, String filename, Binary::View raw)
      : Fs(std::move(vfs))
      , Filename(std::move(filename))
      , Raw(std::move(raw))
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
      String name(filename);
      if (auto blob = self->Fs->Request(name))
      {
        auto* result = new MemoryStream(self->Fs, std::move(name), std::move(blob));
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
    STREAMFILE Vfptr;
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
      const auto done = ::render_vgmstream(safe_ptr_cast<sample_t*>(result.data()), toRender, Tune.get());
      if (Tune->num_samples == Tune->current_sample)
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
    Information(VGMStreamPtr stream)
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
      return VGMStreamPtr(std::move(Stream));
    }

  private:
    const Vfs::Ptr Model;
    mutable VGMStreamPtr Stream;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  VGMStreamPtr TryOpenStream(Vfs::Ptr vfs, int subtrackIndex = -1)
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
      props.SetTitle(stream->stream_name);
      {
        std::array<char, 1024> buf{0};
        ::describe_vgmstream(stream.get(), buf.data(), buf.size());
        props.SetComment(buf.data());
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

    void Resolve(const String& name, Binary::Container::Ptr data) override
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
      if (Delegate = TryCreateModule(Model, Properties))
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
    const char* const Id;
    const char* const Description;
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

      String GetDescription() const override
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

      String GetDescription() const override
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
          if (auto stream = TryOpenStream(std::move(vfs)))
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

  // clang-format off
  const PluginDescription PLUGINS[] =
  {
    //CRI
    // HCA, ~166.2k
    {
      "HCA", "CRI HCA", ".hca",
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
      "ADX", "CRI ADX", ".adx",
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
      "AWB", "CRI Atom Wave Bank", ".awb",
      "'A'F'S'2"           // +0
      "?{4}"               // +4
      "01-ff 000000"       // +8 le32 subsongs count, assume 1..255
      ""_sv,
      PluginType::MULTITRACK
    },
    {
      "CPK", "CRI CPK container", ".awb",
      "'C'P'K' "           // +0
      "?{12}"              // +4
      "'@'U'T'F"           // +10
      ""_sv,
      PluginType::MULTITRACK
    },
    // AAX, ~5.3k
    {
      "AAX", "CRI AAX", ".aax",
      "'@'U'T'F"         // +0 sign
      ""_sv
    },
    // ACB, ~3.7k
    {
      "ACB", "CRI Atom Cursheet Binary", ".acb",
      "'@'U'T'F"           // +0
      " "_sv,              // <- to not to confuse with AAX format
      PluginType::MULTITRACK
    },
    // AIX, ~2.3k
    {
      "AIX", "CRI AIX", ".aix",
      "'A'I'X'F"             // +0
      "????"                 // +4
      "01 00 00 14"          // +8
      "00 00 08 00"          // +c
      ""_sv
    },
    // AHX, ~90 requires MPEG_DECODER
    {
      "AHX", "CRI AHX", ".ahx",
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
      "CSTM", "Nintendo CSTM", ".bcstm",
      "'C'S'T'M"        // +0 sign
      "ff fe"           // +4 BOM
      ""_sv
    },
    // STRM, ~14.4k
    {
      "STRM", "Nintendo STRM", ".strm",
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
      "RSTM", "Nintendo RSTM", ".brstm", // also .brstmspm, but only one example at joshw
      "'R'S'T'M"         // +0 sign
      "feff 00|01 01|00" // +4
      ""_sv
    },
    // FSTM, ~10.8k
    {
      "FSTM", "Nintendo FSTM", ".bfstm",
      "'F'S'T'M"        // +0 sign
      "fe|ff ff|fe"     // +4 BOM
      ""_sv
    },
    // DSP, ~10.4k, more strict checks
    {
      "DSP", "Nintendo ADPCM", ".dsp",
      "00-10 ???"              // +0 be samples up to 0x10000000
      "00-10 ???"              // +4 be nibbles count up to 0x10000000
      "0000 1f-bb ?"           // +8 be samplerate 8k...48k 1f40..bb80
      "00 00|01"               // +c be loop flag 0/1
      "0000"                   // +e format
      "?{8}"                   // +10 loop start/end offset
      "000000 00|02"           // +18 initial offset 0/2
      "?{32}"                  // +1c
      "0000"                   // +3c gain
      ""_sv
    },
    {
      "DSP", "Nintendo ADPCM (LE)", ".adpcm",
      "??? 00-10"              // +0 le samples up to 0x10000000
      "??? 00-10"              // +4 le nibbles count
      "? 1f-bb 0000"           // +8 le samplerate 8k..48k
      "00|01 00"               // +c le loop flag 0/1
      "0000"                   // +e format
      "?{8}"                   // 10 loop start/end offset
      "00|02 000000"           // +18 le initial offset 0/2
      "?{32}"                  // +1c
      "0000"                   // +3c gain
      ""_sv
    },
    // CWAV, ~6.9k
    {
      "CWAV", "Nintendo CWAV", ".bcwav",
      "'C'W'A'V"                // +0 signature
      "fffe4000"                // +4
      "00 00-01 00-01 02"
      ""_sv
    },
    // ADP, ~3k
    {
      "ADP", "Nintendo ADP (raw)", ".adp",
      // up to 10 frames, upper nibble 0..3, lower 0..c, first byte not zero
      "("
      "01-0c|10-1c|20-2c|30-3c" // +0 != 0
      "00-3c"                   // +1
      "01-0c|10-1c|20-2c|30-3c" // +2 == +0
      "00-3c"                   // +3 == +1
      "?{28}"                   // +4  total x20
      "){10}"
      ""_sv
    },
    // BWAV, ~2.9k
    {
      "BWAV", "Nintendo BWAV", ".bwav",
      "'B'W'A'V"                  // +0
      "?{10}"_ss +                // +4
      CHANNELS16LE +              // +e
      "00-01 00"_ss               // +10 codec
    },
    // FWAV, ~2.4k
    {
      "FWAV", "Nintendo FWAV", ".fwav", // also .bfwav,.bfwavnsmbu
      "'F'W'A'V"        // +0 sign
      "fe|ff ff|fe"     // +4 BOM
      ""_sv
    },
    // SWAV, ~1.8k
    {
      "SWAV", "Nintendo SWAV", ".swav",
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
      "RWAV", "Nintendo RWAV", ".rwav",
      "'R'W'A'V"                // +0 signature
      "feff0102"                // +4
      ""_sv
    },
    // THP, ~1k
    {
      "THP", "Nintendo THP", ".thp",
      "'T'H'P 00"      // +0
      "0001 00|10 00"  // +4 version
      ""_sv
    },
    // OPUS, ~650 init_vgmstream_opus with offset=0
    {
      "OPUS", "Nintendo OPUS", ".opus",
      "01000080"       // +0
      "?{4}"           // +4
      "?"_ss +         // +8 version
      CHANNELS8 +      // +9
      ANY16 +          // +a
      SAMPLERATE32LE   // +c
    },
    // AST, ~610
    {
      "AST", "Nintendo AST", ".ast",
      "'S|'M 'T|'R 'R|'T 'M|'S"          // +0 sign
      "????"                             // +4 rest size
      "00 00|01"                         // +8 be codec
      // other fields are really endian-dependent
      ""_sv
    },

    //Sony
    // ADS, ~21.3k
    {
      "ADS", "Sony AudioStream", ".ads", // also ss2
      "'S'S'h'd"               // +0 signature
      "18|20 000000"           // +4 le header size
      "01|02|10 0000 00|80"_ss +// +8 le codec
      SAMPLERATE32LE +         // +c le samplerate
      CHANNELS32LE             // +10 le channels
    },
    // XA, ~16.1k
    {
      "XA", "Sony XA", ".xa",
      "'R'I'F'F"
      "????"
      "'C'D'X'A"
      "'f'm't' "
      ""_sv,
      PluginType::MULTITRACK
    },
    {
      "XA", "Sony XA (raw)", ".xa",
      "00ff{10}00"
      ""_sv,
      PluginType::MULTITRACK
    },
    // VAG, ~13.6k - also filename-based quirks
    {
      "VAG", "Sony VAG", ".vag",
      // +0 sign
      "'p|'V"
      "'G|'A"
      "'A|'G"
      "'V|'1|'2|'i|'p|'V"
      ""_sv
    },
    // XVAG, ~4.6k
    {
      "XVAG", "Sony XVAG", ".xvag",
      "'X'V'A'G"        // +0
      ""_sv,
      PluginType::MULTITRACK
    },
    // MSF, ~3.6k
    {
      "MSF", "Sony MultiStream File", ".msf",
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
      "SGX", "Sony SGX", ".sgx",
      "'S'G'X'D"        // +0
      "?{12}"           // +4
      "'W'A'V'E"        // +10
      "?{512}"          // +14 - to ensure minimal size
      // SGH has the same structure, but smaller size, paired with SGB
    },*/
    // RXWS, ~1.2k
    {
      "XWS", "Sony XWS", ".xws",
      "'R'X'W'S"        // +0
      "?{12}"           // +4
      "'F'O'R'M"        // +10
      ""_sv,
      PluginType::MULTITRACK
    },
    {
      "RXW", "Sony XWS (bad rips)", ".rxw",
      "'R'X'W'S"        // +0
      "?{12}"           // +4
      "'F'O'R'M"        // +10
      "?{26}"_ss +      // +14
      SAMPLERATE32LE,    // +2e
      PluginType::MULTITRACK
    },
    // MIC, ~560
    {
      "MIC", "Sony MultiStream MIC", ".mic",
      "40000000"_ss +             // +0
      ANY32 +                     // +4
      CHANNELS32LE +              // +8
      SAMPLERATE32LE              // +c
    },
    // SXD, ~900 - same problems as in SGX
    // BNK, ~170
    {
      "BNK", "Sony BNK (LE)", ".bnk",
      "03000000"                     // +0
      "02|03 000000"                 // +4
      ""_sv,
      PluginType::MULTITRACK
    },
    {
      "BNK", "Sony BNK (BE)", ".bnk",
      "00000003"                    // +0
      "000000 02|03"                // +4
      ""_sv,
      PluginType::MULTITRACK
    },

    //Audiokinetic
    // WEM, ~18.6k
    {
      "WEM", "Audiokinetic Wwise", ".wem",
      "'R'I'F 'F|'X"           // +0 RIFF/RIFX
      "????"
      "'W|'X"                  // +8 WAVE/XWMA
      "'A|'W"
      "'V|'M"
      "'E|'A"
      ""_sv
    },
    // BNK, ~160
    {
      "BNK", "Audiokinetic Wwise Soundbank Container", ".bnk",
      "'A|'B"
      "'K|'K"
      "'B|'H"
      "'K|'D"
      ""_sv,
      PluginType::MULTITRACK
    },

    //Electronic Arts
    // SCHl (variable), ~13.9k
    {
      "SCHL", "Electronic Arts SCHL", ".",
      "'S 'C|'H"          // +0 sign - SCHl or SH{locale}
      "'H|'E|'F|'G|'D|'I|'S|'E|'M|'R|'J|'J|'P|'B"
      "'l|'N|'R|'E|'E|'T|'P|'S|'X|'U|'A|'P|'L|'R"
      //"????"              // +4 header size
      ""_sv
    },
    // SNR, ~4.5k
    {
      "SNR", "Electronic Arts SNR", ".snr",
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
      ""_sv,
      // Too weak signature, if multifile, may hide another formats
      //PluginType::MULTIFILE  // <- usually additional file name depends on base one
    },
    // SPS, ~2.2k + ~2.4k under MPEG support
    {
      "SPS", "Electronic Arts SPS", ".sps",
      "48 ???"               // +0 header block id + header block size
      // vvvv (0/1) cccc (any) CCCCCC samplerate18...
      "02-0c|0e|12-1c|1e"    // +4 header1 version, codec, channels-1, samplerate
      "???"
      // tt l samples29...
      "%00xxxxxx|%01xxxxxx|%10xxxxxx"
      ""_sv
    },
    // SNU, ~900
    /*{ TODO: enable after fix in hanging calculate_eaac_size
      "SNU", "Electronic Arts SNU", ".snu",
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
      ""_sv
    },*/
    // MPF, ~480 - multitrack+multifile
    /*{
      "MPF", "Electronic Arts MPF", ".mpf",
      "'P|'x"               // +0
      "'F|'D"
      "'D|'F"
      "'x|'P"
      ""_sv
    },*/
    // BNK, ~300
    {
      "BNK", "Electronic Arts BNK", ".bnk",
      "'B'N'K 'l|'b"         // +0
      ""_sv,
      PluginType::MULTITRACK
    },

    //Apple
    // M4A, 11k
    {
      "M4A", "Apple QuickTime Container", ".m4a",
      "000000?"                   // +0
      "'f't'y'p"                  // +4
      ""_sv,
      PluginType::MULTITRACK
    },
    // AIFF, ~2k
    {
      "AIFF", "Apple Audio Interchange File Format", ".aiff",
      "'F'O'R'M"       // +0
      "????"           // +4
      "'A'I'F'F"       // +8
      ""_sv
    },
    // AIFC, ~1.3k
    {
      "AIFC", "Apple Compressed Audio Interchange File Format", ".aifc",
      "'F'O'R'M"       // +0
      "????"           // +4
      "'A'I'F'C"       // +8
      ""_sv
    },
    // CAF, ~620
    {
      "CAF", "Apple Core Audio Format File", ".caf",
      "'c'a'f'f"       // +0
      "00010000"       // +4
      ""_sv
    },

    //Ubisoft
    // singletrack BAO, ~6.1k, multifile
    // atomic with no subtracks
    {
      "BAO", "Ubisoft BAO (atomic)", ".bao",
      "01|02"
      "1b   |1f      |21|22         |23|25         |27|28"
      "01|02|00      |00|00|ff      |00|01         |01|03"
      "00   |08|10|11|0c|0d|15|1b|1e|08|08|0a|19|1d|02|03"
      ""_sv,
      PluginType::MULTIFILE
    },
    // RAK, ~3.5k TODO: merge after parallel formats implementation
    {
      "RAK", "Ubisoft RAKI", ".rak",
      "'R'A'K'I"                  // +0
      "?{28}"                     // +4
      "'f'm't' "                  // +20
      ""_sv
    },
    {
      "RAK", "Ubisoft RAKI (modified)", ".rak",
      "????"                      // +0
      "'R'A'K'I"                  // +0+4
      "?{28}"                     // +4+4
      "'f'm't' "                  // +20+4
      ""_sv
    },
    // LWAV, LyN ~1.7k, Jade ~840
    {
      "LWAV", "Ubisoft RIFF", ".lwav",
      "'R'I'F'F"       // +0
      "????"           // +4
      "'W'A'V'E"       // +8
      ""_sv
    },
    {
      "LWAV", "Ubisoft LyN", ".lwav",
      "'L'y'S'E"        // +0
      "?{16}"           // +4
      "'R'I'F'F"        // +14
      "'W'A'V'E"        // +18
      ""_sv
    },
    // CKD, ~1k, endian-dependend fields
    {
      "CKD", "Ubisoft CKD", ".ckd",
      "'R'I'F'F"        // +0
      "????"            // +4
      "'W'A'V'E"        // +8
      "?{8}"            // +c
      "00   |01| 02|55|66" // +14 BE+LE
      "02|55|66| 00   |01"
      ""_sv
    },

    //Konami
    // BMP, ~4.9k
    {
      "BMP", "Konami BMP", ".bin",
      "'B'M'P 00"_ss +      // +0 signature
      SAMPLES32BE +         // +4 samples count
      "?{8}"                // +8
      "02 ???"_ss +         // +10 channels, always stereo
      SAMPLERATE32BE        // +14 samplerate
    },
    // 2DX9, ~4.7k
    {
      "2DX9", "Konami 2DX9", ".2dx9",
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
      "IFS", "Konami Arcade Games Container", ".ifs",
      "6cad8f89"        // +0
      "0003"            // +4
      ""_sv,
      PluginType::MULTITRACK
    },
    // SVAG, ~1k
    {
      "SVAG", "Konami SVAG", ".svag",
      "'S'v'a'g"             // +0
      "?{4}"_ss +            // +4
      SAMPLERATE32LE +       // +8
      CHANNELS16LE           // +c
    },
    // SD9, ~470
    {
      "SD9", "Konami SD9", ".sd9",
      "'S'D'9 00"            // +0
      "?{28}"                // +4
      "'R'I'F'F"             // +20
      "????"                 // +24
      "'W'A'V'E"             // +28
      "'f'm't' "             // +2c
      ""_sv
    },
    // KCEY, ~200
    {
      "KCEY", "Konami KCE Yokohama", ".kcey",
      "'K'C'E'Y"_ss +        // +0
      ANY32 +                // +4
      CHANNELS32BE +         // +8
      SAMPLES32BE            // +c
    },
    // MSF, ~140
    {
      "MSF", "Konami MSF", ".msf",
      "'M'S'F'C"             // +0
      "00000001"_ss +        // +4 be codec
      CHANNELS32BE +         // +8
      SAMPLERATE32BE         // +c
    },
    // DSP, ~370 - too weak structure

    //Koei
    // KOVS/KVS, ~9.8k
    {
      "KVS", "Koei Tecmo KVS", ".kvs",
      "'K'O'V'S"
      ""_sv
    },
    // KTSS, ~2.5k
    {
      "KTSS", "Koei Tecmo KTSS", ".ktss",
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
      "MIC", "Koei Tecmo MIC", ".mic",
      "00 08 00 00"_ss +     // +0 le start offset = 0x800
      SAMPLERATE32LE +       // +4
      CHANNELS32LE           // +8
    },
    // DSP, ~460
    {
      "DSP", "Koei Tecmo WiiBGM", ".dsp",
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
      "SAB", "Square Enix SAB", ".sab",
      "'s'a'b'f"       // +0
      ""_sv,
      PluginType::MULTITRACK
    },
    // AKB, ~2.1k
    {
      "AKB", "Square Enix AKB", ".akb",
      "'A'K'B' "        // +0
      "?{8}"            // +4
      "02|05|06"_ss +   // +c codec
      CHANNELS8 +       // +d
      SAMPLERATE16LE +  // +e
      SAMPLES32LE       // +10
    },
    // MAB, ~1.7k
    {
      "MAB", "Square Enix MAB", ".mab",
      "'m'a'b'f"       // +0
      ""_sv,
      PluginType::MULTITRACK
    },
    // AKB, ~1k
    {
      "AKB", "Square Enix AKB2", ".akb",
      "'A'K'B'2"        // +0
      "?{8}"            // +4
      "01|02"           // +c table count
      ""_sv,
      PluginType::MULTITRACK
    },
    // SCD, ~980
    {
      "SCD", "Square Enix SCD", ".scd",
      "'S'E'D'B'S'S'C'F"   // +0
      ""_sv,
      PluginType::MULTITRACK
    },
    // BGW, ~330
    {
      "BGW", "Square Enix BGW", ".bgw",
      "'B'G'M'S't'r'e'a'm 000000" // +0
      "00|03 000000"              // +c codec
      ""_sv
    },

    //Namco
    // IDSP, ~2.1k
    // init_vgmstream_nub_idsp
    {
      "IDSP", "Namco IDSP (NUB container)", ".idsp",
      "'i'd's'p"          // +0
      "?{184}"            // +4
      "'I'D'S'P"          // +0+188
      "????"              // +4+188
      "00 00 00 01-08"    // +8+188, channels
      ""_sv
    },
    // NPS, ~770
    {
      "NPS", "Namco NPSF", ".nps",
      "'N'P'S'F"               // +0 signature
      "?{8}"_ss +              // +4
      CHANNELS32LE +           // +c le channels
      "?{8}"_ss +              // +10
      SAMPLERATE32LE           // +18
    },
    // NAAC, ~370
    {
      "NAAC", "Namco AAC", ".naac",
      "'A'A'C' "          // +0
      "01000000"_ss +     // +4
      ANY32 +             // +8
      SAMPLERATE32LE +    // +c
      SAMPLES32LE         // +10
    },
    // CSTR, ~300
    {
      "DSP", "Namco CSTR", ".dsp",
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
      "DSB", "Namco DSB", ".dsb",
      "'D'S'S'B"               // +0
      "?{60}"                  // +4
      "'D'S'S'T"               // +40
    },*/
    // NUB,NUB2,NUS3 a lot subformats
    {
      "NUS3", "Namco nuSound container v3", ".nus3bank",
      "'N'U'S'3"          // +0
      "????"              // +4
      "'B'A'N'K'T'O'C' "  // +8
      ""_sv,
      PluginType::MULTITRACK
    },
    {
      "NUB", "Namco nuSound container", ".nub",
      // +0
      "00   |01"
      "02   |02"
      "00|01|01"
      "00   |00"
      // +4
      "00000000"
      ""_sv,
      PluginType::MULTITRACK
    },

    //Capcom
    // MCA, ~2.1k
    {
      "MCA", "Capcom MCA", ".mca",
      "'M'A'D'P"        // +0 sign
      "????"_ss +
      CHANNELS8 +       // +8 channels count
      "???"_ss +
      SAMPLES32LE +     // +c le samples up to 1000000000
      SAMPLERATE16LE    // +10 le samplerate from 256hz
    },
    // AUS, ~1.1k
    {
      "AUS", "Capcom AUS", ".aus",
      "'A'U'S' "        // +0
      "?{4}"_ss +       // +4
      SAMPLES32LE +     // +8
      CHANNELS32LE +    // +c
      SAMPLERATE32LE    // +10
    },
    // LOPUS, ~500
    {
      "LOPUS", "Capcom OPUS container", ".lopus",
      SAMPLES32LE +     // +0
      "01|02|06 000000" // +4
      "?{16}"           // +8
      "0000"_ss         // +18
    },
    // DSPW, ~310
    {
      "DSPW", "Capcom DSPW", ".dspw",
      "'D'S'P'W"        // +0
      ""_sv
    },
    // DVI, ~100
    {
      "DVI", "Capcom IDVI", ".dvi",
      "'I'D'V'I"_ss +   // +0
      CHANNELS32LE +    // +4
      SAMPLERATE32LE    // +8
    },

    //Crystal Dynamics
    // MUL, ~1.5k
    {
      "MUL", "Crystal Dynamics MUL", ".mul",
      // +0 samplerate4 8000..48000 (#1f40..#bb80)
      // +c channels4 1..8
      "0000 1f-bb ?" // +0
      "?{8}"         // +4
      "000000 01-08" // +c
      ""_sv
    },
    {
      "MUL", "Crystal Dynamics MUL (BE)", ".mul",
      // +0 samplerate4 8000..48000 (#1f40..#bb80)
      // +c channels4 1..8
      "? 1f-bb 0000" // +0
      "?{8}"         // +4
      "01-08 000000" // +c
      ""_sv
    },

    //Microsoft
    // XMA, ~48k
    {
      "XMA", "Microsoft XMA", ".xma",
      "'R'I'F'F"         // +0
      ""_sv
    },
    // XWB, ~8.5k
    {
      "XWB", "Microsoft XACT Wave Bank", ".xwb",
      // +0 sign
      "'W|'D"
      "'B|'N"
      "'N|'B"
      "'D|'W"
      ""_sv,
      PluginType::MULTITRACK
    },
    // XWM, ~900
    {
      "XWM", "Microsoft XWMA", ".xwm",
      "'R'I'F'F"          // +0
      "????"              // +4
      "'X'W'M'A"          // +8
      ""_sv,
      PluginType::MULTITRACK
    },
    // XNB, ~590
    {
      "XNB", "Microsoft XNA Game Studio XNB", ".xnb",
      "'X'N'B ?"          // +0
      "04|05"             // +4
      ""_sv,
      PluginType::MULTIFILE // also multitrack, but no samples
    },

    //Hudson
    // PDT, ~440
    {
      "PDT", "Hudson Splitted Stream container", ".pdt",
      "'P'D'T' "           // +0
      "'D'S'P' "           // +4
      "'H'E'A'D"           // +8
      "'E'R"               // +c
      "01|02 00"           // +e channels
      ""_sv
    },
    // PDT, ~440
    {
      "PDT", "Hudson Stream Container", ".pdt",
      "0001 ??"            // +0 be 1
      "000000 02|04"       // +4 be 2/4
      "00007d00"           // +8 be 0x7d00
      "000000 02|04"       // +c be 2/04
      ""_sv,
      PluginType::MULTITRACK
    },
    // H4M, ~170
    {
      "H4M", "Hudson HVQM4", ".h4m",
      "'H'V'Q'M'4' '1'."   // +0
      "'3|'5 000000"       // +8
      "?{4}"               // +c
      "00000044"           // +10
      ""_sv,
      PluginType::MULTITRACK
    },

    //RAD Game Tools
    // BIK, ~19.9k
    {
      "BIK", "RAD Game Tools Bink", ".bik",
      // +0 signature
      "'B|'K"
      "'I|'B"
      "'K|'2"
      "?"
      "????"         // +4 filesize
      "?? 00-0f 00"  // +8 le frames up to 0x100000
      "?{28}"        // +c
      "01-ff 000000" // +28 le total subsongs 1..255
      ""_sv,
      PluginType::MULTITRACK
    },
    // SMK, ~190
    {
      "SMK", "RAD Game Tools SMACKER", ".smk",
      "'S'M'K '2|'4"   // +0
      "?{8}"           // +4
      "?? 00-0f 00"    // +c le frames up to 0x100000
      ""_sv
      //PluginType::MULTITRACK <- really no multitrack samples...
    },

    //Radical
    // RSD, ~4.4k
    {
      "RSD", "Radical RSD", ".rsd",
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
      "P3D", "Radical P3D", ".p3d",
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
      ""_sv
    },

    //Retro Studios
    // RFRM, ~200
    {
      "RFRM", "Retro Studios RFRM", ".csmp",
      "'R'F'R'M"               // +0
      "?{16}"                  // +4
      "'C'S'M'P"               // +14
      ""_sv
    },
    // DSP, ~150
    {
      "DSP", "Retro Studios RS03", ".dsp",
      "'R'S 00 03"             // +0 signature
      "00 00 00 01|02"_ss +    // +4 be channels count
      SAMPLES32BE +            // +8 be samples up to 1000000000
      SAMPLERATE32BE           // +c be samplerate up to 192000
    },

    //Banpresto
    // WMSF, ~600
    {
      "WMSF", "Banpresto MSF wrapper", ".msf",
      "'W'M'S'F"            // +0
    },
    // 2MSF, ~290
    {
      "2MSF", "Banpresto RIFF wrapper", ".at9",
      "'2'M'S'F"            // +0
    },

    //Bizzare Creations
    // BAF, ~290
    {
      "BAF", "Bizzare Creations bank file", ".baf",
      "'B'A'N'K"             // +0
      "?{4}"                 // +4
      "00|03|04|05"          // +8 be/le 3/4/5
      "0000"
      "00|03|04|05"
      ""_sv,
      PluginType::MULTITRACK
    },
    {
      "BAF", "Bizzare Creations band file (bad rips)", ".baf",
      "'W'A'V'E"             // +0
      "0000004c"             // +4
      // "DATA" @ 4c
      ""_sv,
      PluginType::MULTITRACK
    },

    //Other
    // FSB, v1 - 30, v2 - no, v3 - 3.9k, v4 - 19.4k, v5 - 203.8k, some of them are already supported
    {
      "FSB", "FMOD Sample Bank (v1-v5)", ".fsb",
      "'F'S'B '1-'5"               // +0 sign
      ""_sv,
      PluginType::MULTITRACK
    },
    // GENH, ~15.3k
    {
      "GENH", "Generic Header", ".genh",
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
      "MIB", "Headerless PS-ADPCM", ".mib",
      // Px F <any> (16 total) P=0..5 F=0..7 up to 1k total
      "("
      "00-5f"
      "00-07"
      "?{14}"
      "){16}"
    },*/
    // IDSP, ~3.6k= ~2.1k Namco, ~720 Next level, ~720 Traveller and other
    {
      "IDSP", "Various IDSP", ".idsp",
      "'I'D'S'P"          // +0
      ""_sv
    },
    // ACM, ~3.1k
    {
      "ACM", "InterPlay ACM", ".acm",
      "97 28 03 01"               // +0
      ""_sv
    },
    // OPUS, ~3k
    {
      "OPUS", "Various OPUS containers", ".opus",
      "'O'P'U'S"    // +0
      ""_sv
    },
    // XA, ~2.8k conflicts with 'Nintendo ADPCM (LE)'
    {
      "XA", "Maxis XA", ".xa",
      "'X'A"                      // +0
      "?{6}"                      // +2
      "01 00"_ss +                // +8 format tag
      CHANNELS16LE +              // +a
      SAMPLERATE32LE              // +c
    },
    // VPK, ~2.7k
    {
      "VPK", "SCE America VPK", ".vpk",
      "' 'K'P'V"                  // +0
      "?{12}"_ss +                // +4
      SAMPLERATE32LE +            // +10
      CHANNELS32LE                // +14
    },
    // MUSX, ~2.3k
    {
      "MUSX", "Eurocom MUSX", ".musx",
      "'M'U'S'X"       // +0
      "????"           // +4
      "01|c9|04-06|0a" // +8 version
      ""_sv,
      PluginType::MULTITRACK
    },
    // RPGMVO, ~1.7k
    {
      "RPGMVO", "Encrypted OGG Vorbis", ".rpgmvo",
      "'R'P'G'M'V 000000" // +0
      ""_sv
    },
    {
      "LOGG", "Encrypted OGG Vorbis", ".logg",
      "2c|4c|04|4f"
      "44|32|86|75"
      "44|53|86|6f"
      "30|44|c5|71"
      ""_sv
    },
    {
      "LOGG", "Corrupted OGG Vorbis", ".logg",
      // TODO: support negation in format notation
      "00-4e|50-ff"  // !'O  +0
      "00-66|68-ff"  // !'g
      "00-66|68-ff"  // !'g
      "00-52|54-ff"  // !'S
      "?{54}"
      "'O'g'g'S"     // +3a
      ""_sv
    },
    // OPUS, ~1.7k
    {
      "OPUS", "OGG Opus", ".opus",
      "'O'g'g'S"     // +0
      "?{24}"        // +4
      "'O'p'u's'H'e'a'd" // +1c assume one-lace first page
      ""_sv
    },
    // RWS, ~1.7k
    {
      "RWS", "RenderWare Stream", ".rws",
      "0d080000"       // +0
      "????"           // +4 file size
      "????"           // +8
      "0e080000"       // +c
      ""_sv,
      PluginType::MULTITRACK
    },
    // XWC, ~1400
    {
      "XWC", "Starbreeze XWC", ".xwc",
      "00"
      "03|04"
      "00"
      "00"
      "00"
      "90"
      "00"
      "00"
      ""_sv
    },
    // AUD (new), ~1.2k
    {
      "AUD", "Westwood Studios AUD", ".aud",
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
      "BSND", "id Tech 5 audio", ".bsnd",
      "'b's'n'f 00"    // +0
      "00000100"       // +5
    },*/
    // HPS, ~1k
    {
      "HPS", "HAL Labs HALPST", ".hps",
      "' 'H'A'L'P'S'T 00"_ss + // +0 signature
      ANY32 +                  // +8
      CHANNELS32BE             // +c be channels count
    },
    // MUSC, ~850
    {
      "MUSC", "Krome MUSC", ".musc",
      "'M'U'S'C"_ss +          // +0
      ANY16 +                  // +4
      SAMPLERATE16LE           // +6
    },
    // SVAG, ~840
    {
      "SVAG", "SNK SVAG", ".svag",
      "'V'A'G'm"_ss +         // +0
      ANY32 +                 // +4
      SAMPLERATE32LE +        // +8
      CHANNELS32LE            // +c
    },
    // AWC, ~830
    {
      "AWC", "Rockstar AWC", ".awc",
      "'A|'T"        // +0
      "'D|'A"
      "'A|'D"
      "'T|'A"
      ""_sv,
      PluginType::MULTITRACK
    },
    // ZSS, ~800
    {
      "ZSS", "Z-Axis ZSND", ".zss",
      "'Z'S'N'D"          // +0
      // +4 codec
      "'P|'X|'P|'G"
      "'C|'B|'S|'C"
      "' |'O|'2|'U"
      "' |'X|' |'B"
      ""_sv,
      PluginType::MULTITRACK
    },
    // SEB, ~740
    {
      "SEB", "Game Arts SEB", ".seb",
      "01|02 000000"_ss +     // +0 channels count
      SAMPLERATE32LE          // +4
    },
    // SEG, ~700
    {
      "SEG", "Stormfront SEG", ".seg",
      "'s'e'g 00"             // +0
      // +4
      "'p|'x|'w|'p|'x"
      "'s|'b|'i|'c|'b"
      "'2|'x|'i|'_|'3"
      "00"
      ""_sv
    },
    // WAVE, ~660
    {
      "WAVE", "EngineBlack WAVE", ".wave",
      "e5|fe"                    // +0
      "b7|ec"
      "ec|b7"
      "fe|e5"
      "00|01 ???"                // +4 version
      ""_sv
    },
    // IMUSE, ~650
    {
      "IMUS", "LucasArts iMUSE", ".imc",
      "'C|'M"                    // +0
      "'O|'C"
      "'M"
      "'P"
      ""_sv
    },
    // VGS, ~650
    {
      "VGS", "Princess Soft VGS", ".vgs",
      "'V'G'S 00"             // +0
      "?{12}"_ss +            // +4
      SAMPLERATE32BE          // +c
    },
    // SMP, ~640
    {
      "SMP", "Infernal Engine SMP", ".smp",
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
      "AAC", "tri-Ace ASKA engine", ".aac",
      "'A|' "               // +0
      "'A|'C"
      "'C|'A"
      "' |'A"
      // v1 "DIR "@0x30
      // v2 no stable parts
      // v3 no stable parts
      ""_sv,
      PluginType::MULTITRACK
    },
    // NXA, ~510
    {
      "NXA", "Entergram NXA", ".nxa",
      "'N'X'A'1"             // +0
      "01|02 000000"         // +4 type
      ""_sv
    },
    // STR, ~500
    {
      "STR", "Sega Stream Asset Builder STR", ".str",
      ANY32 +                 // +0
      SAMPLERATE32LE +        // +4
      "04|10 000000"          // +8
      "?{201}"                // +c
      "'S'e'g'a"              // +d5
      ""_ss
    },
    // IKM, ~470
    {
      "IKM", "MiCROViSiON container", ".ikm",
      "'I'K'M 00"             // +0
      "?{28}"                 // +4
      "00|01|03 000000"       // +20
      ""_sv
    },
    // SPSD, ~360
    {
      "SPSD", "Naomi SPSD", ".spsd",
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
      "HWAS", "Vicarious Visions HWAS", ".hwas",
      "'s'a'w'h"_ss +          // +0
      ANY32 +                  // +4 usually 0x2000, 0x4000, 0x8000, but for any...
      SAMPLERATE32LE +         // +8
      "01000000"_ss            // +c channels
    },
    // VXN, ~300
    {
      "VXN", "Gameloft VXN", ".vxn",
      "'V'o'x'N"            // +0
      ""_sv,
      PluginType::MULTITRACK
    },
    // GCA, ~260
    {
      "GCA", "Terminal Reality GCA", ".gca",
      "'G'C'A'1"               // +0
      "?{38}"_ss +             // +4
      SAMPLERATE32BE           // +2a
    },
    // STM, ~250
    {
      "STM", "Intelligent Systems STM", ".stm",
      "0200"_ss +      // +0
      SAMPLERATE16BE + // +2
      CHANNELS32BE
    },
    // ISWS, ~250
    {
      "ISWS", "Sumo Digital iSWS", ".isws",
      "'i'S'W'S"        // +0
    },
    // XWV, ~250
    {
      "XWV", "feelplus XWAV", ".xwv",
      // +0
      "'V|'X"
      "'A|'W"
      "'W|'A"
      "'X|'V"
    },
    // ATX, ~240, filename-based
    /*{
      "ATX", "Media Vision ATX", ".atx",
      "'A'P'A'3"        // +0
      "?{24}"           // +4
      "0000"            // +1c
      "01|02 00"        // +1e
    },*/
    // PONA, ~230
    {
      "PONA", "Policenauts BGM", ".pona",
      // be 0x13020000(3do)/0x00000800(psx)
      "13|00"
      "02|00"
      "00|08"
      "00|00"
      ""_sv
    },
    // SADL, ~230
    {
      "SAD", "Procyon Studio SADL", ".sad",
      "'s'a'd'l"                      // +0
      "?{47}"                         // +4
      "%0000xxxx|%0111xxxx|%1011xxxx" // +33 flags
      ""_sv
    },
    // ADS, ~220
    {
      "ADS", "Midway ADS", ".ads",
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
      "STR", "3DO STR", ".str",
      "'C|'S|'S"
      "'T|'N|'H"
      "'R|'D|'D"
      "'L|'S|'R"
      ""_sv
    },
    // CKS, ~180
    {
      "CKS", "Cricket Audio Stream", ".cks",
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
      "CAF", "tri-Crescendo CAF", ".caf",
      "'C'A'F' "               // +0
      ""_sv
    },
    // ISD, ~170
    {
      "ISD", "Inti Creates OGG Vorbis", ".isd",
      "af|0f|0f"
      "67|e7|a7"
      "87|87|47"
      "53|d3|53"
      ""_sv
    },
    // YDSP, ~170
    {
      "YDSP", "Yuke DSP", ".ydsp",
      "'Y'D'S'P"_ss +          // +0
      ANY32 +                  // +4
      ANY32 +                  // +8
      SAMPLERATE32BE +         // +c
      CHANNELS16BE             // +10
    },
    // RSD, ~150, decoded only
    {
      "RSD", "RedSparc RSD", ".rsd",
      "'R'e'd'S'p'a'r'k"      // +0
      "?{52}"_ss +            // +8
      SAMPLERATE32LE +        // +3c
      SAMPLES32LE +           // +40
      "?{10}"_ss +            // +44
      CHANNELS8               // +4e
    },
    // ADP, ~145
    {
      "ADP", "Nex Entertainment NXAP", ".adp",
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
      "ADP", "Xilam DERF", ".adp",
      "'D'E'R'F"              // +0
      "01|02 000000"          // +4 le channels, up to 2
      ""_sv
    },
    // SDF, ~45
    {
      "SDF", "Beyond Reality SDF", ".sdf",
      "'S'D'F 00"                     // +0
      "03000000"                      // +4
      ""_sv
    },
    // ZSD, ~40
    {
      "ZSD", "ZSD container", ".zsd",
      "'Z'S'D 00"                     // +0
      "?{12}"_ss +                    // +4
      SAMPLERATE32LE +                // +10
      ANY32 +                         // +14
      SAMPLES32LE                     // +18, always mono
    },
    // MTX, ~? - should be decoder...
    {
      "MTX", "Matric Software container", ".bin",
      "'F|'m"                  // +0
      "'F|'t"
      "'D|'x"
      "'L|'s"
      ""_sv
    },

    //  FFMPEG-based
    // APE, ~20k
    {
      "APE", "Monkey's Audio", ".ape",
      "'M'A'C' "            // +0
      ""_sv
    },
    // ASF, ~10k - TODO: multitrack
    {
      "ASF", "Windows Media Audio", ".asf",
      "30 26 B2 75 8E 66 CF 11 A6 D9 00 AA 00 62 CE 6C" // +0 asf guid
      ""_sv,
      PluginType::MULTITRACK
    },
    // TAK, ~7.8k
    {
      "TAK", "Tom's lossless Audio Kompressor", ".tak",
      "'t'B'a'K"      // +0
      ""_sv
    },
    // AAC, ~4k
    {
      "AAC", "Advanced Audio Coding (raw ADTS)", ".aac",
      // https://wiki.multimedia.cx/index.php/ADTS
      "%11111111 %1111x00x" // +0 w & 0xfff6 == 0xfff0
      ""_sv
    },
    // OMA, ~2.5k
    {
      "OMA", "Sony OpenMG audio", ".oma",
      "'e'a'3"
      ""_sv
    },
    // MPC, ~2k
    {
      "MPC", "Musepack", ".mpc",
      "'M"
      "'P"
      "'+   |'C"
      "07|17|'K"
      ""_sv
    },
    // AC3, ~650
    {
      "AC3", "Dolby AC-3", ".ac3",
      "0b77"          // +0 - also little endian?
      "??"            // +2 crc
      // see ff_ac3_parse_header
      // samplerate2 10/01/00
      // framesize6 0..37
      "00-25|40-65|80-a5"
      // bitstream id5 0..10 (normal ac3) mode3 any
      // %00000xxx-%01010xxx
      "00-57"
      ""_sv
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
