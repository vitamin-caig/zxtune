/**
 *
 * @file
 *
 * @brief  vgmstream-based formats support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/external/vgmstream.h"

#include "formats/chiptune/common/container.h"
#include "module/players/duration.h"
#include "module/players/properties_helper.h"
#include "module/players/properties_meta.h"
#include "module/players/streaming.h"

#include "binary/container_base.h"
#include "binary/crc.h"
#include "binary/format_factories.h"
#include "debug/log.h"
#include "formats/multitrack/container.h"
#include "math/numeric.h"
#include "module/additional_files.h"
#include "sound/resampler.h"
#include "strings/sanitize.h"

#include "contract.h"
#include "error_tools.h"
#include "make_ptr.h"
#include "static_string.h"
#include "string_view.h"

#include <algorithm>
#include <utility>

extern "C"
{
// clang-format off
#include "3rdparty/vgmstream/config.h"
#include "3rdparty/vgmstream/vgmstream.h"
#include "3rdparty/vgmstream/base/plugins.h"
  // clang-format on
}

namespace Module::VGMStream
{
  const Debug::Stream Dbg("Module::VGMStream");

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

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(VGMStreamPtr tune, uint_t samplerate)
      : Tune(std::move(tune))
      , SamplesPerFrame(FRAME_DURATION.Get() * Tune->sample_rate / FRAME_DURATION.PER_SECOND)
      , Target(Sound::CreateResampler(Tune->sample_rate, samplerate))
      , Channels(Tune->channels)
    {
      ::vgmstream_mixing_autodownmix(Tune.get(), Sound::Sample::CHANNELS);
      ::vgmstream_mixing_enable(Tune.get(), SamplesPerFrame, nullptr, &Channels);
      Dbg("Rendering {}Hz/{}ch -> {}Hz/{}ch", Tune->sample_rate, Tune->channels, samplerate, Channels);
    }

    State GetState() const override
    {
      return {.At = Time::AtMillisecond() + Time::Milliseconds::FromRatio(Tune->current_sample, Tune->sample_rate),
              .Total = Time::Milliseconds::FromRatio(Tune->pstate.play_duration, Tune->sample_rate),
              .LoopCount = static_cast<uint_t>(Tune->loop_count)};
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
    const uint_t SamplesPerFrame;
    const Sound::Converter::Ptr Target;
    int Channels;
  };

  Information MakeInformation(const VGMSTREAM& stream)
  {
    return CreateTimedInfo(
        Time::Milliseconds::FromRatio(stream.num_samples, stream.sample_rate),
        Time::Milliseconds::FromRatio(stream.num_samples - stream.loop_start_sample, stream.sample_rate));
  }

  class Holder : public Module::Holder
  {
  public:
    Holder(Vfs::Ptr model, VGMStreamPtr stream, Parameters::Accessor::Ptr props)
      : Model(std::move(model))
      , Stream(std::move(stream))
      , Properties(std::move(props))
    {}

    Information GetModuleInformation() const override
    {
      return MakeInformation(*Stream);
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

    Information GetModuleInformation() const override
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

  class Factory : public ExternalParsingFactory
  {
  public:
    Factory(StringView id, StringView description, StringView suffix, PluginType type)
      : Id(id)
      , Description(description)
      , Suffix(suffix)
      , Type(type)
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Formats::Chiptune::Container& container,
                             Parameters::Container::Ptr properties) const override
    {
      try
      {
        // assume all dump is used
        Dbg("Trying {}", Description);
        auto vfs = MakePtr<Vfs>(Suffix, container.GetSubcontainer(0, container.Size()));
        if (auto singlefile = TryCreateModule(vfs, properties))
        {
          return singlefile;
        }
        else if (Type == PluginType::MULTIFILE && vfs->HasUnresolved())
        {
          Dbg("Try to process as multifile module");
          return MakePtr<MultifileHolder>(std::move(vfs), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create {}: {}", Id, e.what());
      }
      return {};
    }

  private:
    const StringView Id;
    const StringView Description;
    const String Suffix;
    const PluginType Type;
  };

  ExternalParsingFactory::Ptr CreateFactory(StringView id, StringView description, StringView suffix, PluginType type)
  {
    return MakePtr<Factory>(id, description, suffix, type);
  }

  class Multitrack : public MultitrackFactory
  {
  public:
    Multitrack(StringView id, StringView description, StringView suffix)
      : Id(id)
      , Description(description)
      , Suffix(suffix)
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Formats::Multitrack::Container& container,
                             Parameters::Container::Ptr properties) const override
    {
      try
      {
        Dbg("Trying {}", Description);
        // assume all dump is used
        auto vfs = MakePtr<Vfs>(Suffix, container.GetSubcontainer(0, container.Size()));
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
        Dbg("Failed to create {}: {}", Id, e.what());
      }
      return {};
    }

  private:
    const StringView Id;
    const StringView Description;
    const String Suffix;
  };

  MultitrackFactory::Ptr CreateMultitrackFactory(StringView id, StringView description, StringView suffix)
  {
    return MakePtr<Multitrack>(id, description, suffix);
  }
}  // namespace Module::VGMStream

namespace Formats::Multitrack
{
  namespace VGMStream
  {
    class Container : public Binary::BaseContainer<Multitrack::Container, Chiptune::Container>
    {
    public:
      Container(const Binary::Container& data, uint_t totalTracks, uint_t trackIndex)
        : BaseContainer(Chiptune::CreateCalculatingCrcContainer(data))
        , TotalTracks(totalTracks)
        , TrackIndex(trackIndex)
      {}

      uint_t Checksum() const override
      {
        return Delegate->Checksum();
      }

      uint_t FixedChecksum() const override
      {
        return Delegate->FixedChecksum();
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

    class Decoder : public Multitrack::Decoder
    {
    public:
      Decoder(StringView format, StringView description, StringView suffix)
        : Description(description)
        , Suffix(suffix)
        , Format(Binary::CreateMatchOnlyFormat(format))
      {}

      StringView GetDescription() const override
      {
        return Description;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (Check(rawData))
        {
          auto vfs = MakePtr<Module::VGMStream::Vfs>(Suffix, rawData.GetSubcontainer(0, rawData.Size()));
          if (auto stream = TryOpenStream(vfs))
          {
            // Formats clashing
            if (stream->num_streams)
            {
              return MakePtr<Container>(rawData, stream->num_streams, 0);
            }
            else
            {
              return MakePtr<Container>(rawData, 1, -1);
            }
          }
        }
        return {};
      }

    private:
      const StringView Description;
      const String Suffix;
      const Binary::Format::Ptr Format;
    };
  }  // namespace VGMStream

  Decoder::Ptr CreateVGMStreamDecoder(StringView format, StringView description, StringView suffix)
  {
    return MakePtr<VGMStream::Decoder>(format, description, suffix);
  }
}  // namespace Formats::Multitrack
