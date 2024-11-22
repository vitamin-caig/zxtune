/**
 *
 * @file
 *
 * @brief  ASAP-based formats support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/decoders.h"
#include "formats/multitrack/decoders.h"
#include "module/players/duration.h"
#include "module/players/platforms.h"
#include "module/players/properties_helper.h"
#include "module/players/streaming.h"

#include "binary/format_factories.h"
#include "core/core_parameters.h"
#include "core/plugin_attrs.h"
#include "debug/log.h"
#include "math/numeric.h"
#include "module/attributes.h"
#include "parameters/tracking_helper.h"
#include "sound/resampler.h"
#include "strings/optimize.h"
#include "tools/xrange.h"

#include "byteorder.h"
#include "contract.h"
#include "error.h"
#include "make_ptr.h"
#include "string_view.h"

#include "3rdparty/asap/asap.h"

namespace Module::ASAP
{
  const Debug::Stream Dbg("Core::ASAPSupp");

  using TimeUnit = Time::Millisecond;

  class AsapTune
  {
  public:
    using Ptr = std::shared_ptr<AsapTune>;

    AsapTune(StringView id, Binary::View data, int track)
      : Module(::ASAP_New())
      , Track(track)
    {
      CheckError(::ASAP_Load(Module, "dummy."s.append(id).c_str(), static_cast<const unsigned char*>(data.Start()),
                             data.Size()),
                 "ASAP_Load");
      Reset();  // required for subsequential calls
      Info = ::ASAP_GetInfo(Module);
      Require(Track == ::ASAPInfo_GetDefaultSong(Info));
      Channels = ::ASAPInfo_GetChannels(Info);
      Dbg("Track {}, {} channels", Track, Channels);
      Require(Channels == 1 || Channels == 2);
    }

    ~AsapTune()
    {
      ::ASAP_Delete(Module);
    }

    int GetSongsCount() const
    {
      return ::ASAPInfo_GetSongs(Info);
    }

    void FillDuration(const Parameters::Accessor& params)
    {
      if (::ASAPInfo_GetDuration(Info, Track) <= 0)
      {
        const auto duration = GetDefaultDuration(params);
        ::ASAPInfo_SetDuration(const_cast<ASAPInfo*>(Info), Track, duration.CastTo<TimeUnit>().Get());
      }
    }

    Time::Duration<TimeUnit> GetDuration() const
    {
      return Time::Duration<TimeUnit>{static_cast<unsigned>(::ASAPInfo_GetDuration(Info, Track))};
    }

    void GetProperties(Binary::View data, PropertiesHelper& props) const
    {
      const auto title = Strings::OptimizeAscii(::ASAPInfo_GetTitle(Info));
      const auto author = Strings::OptimizeAscii(::ASAPInfo_GetAuthor(Info));
      const auto date = Strings::OptimizeAscii(::ASAPInfo_GetDate(Info));

      props.SetTitle(title);
      props.SetAuthor(author);
      props.SetDate(date);
      props.SetStrings(GetInstruments(data));
      props.SetChannels(GetChannels());
    }

    void Reset()
    {
      CheckError(::ASAP_PlaySong(Module, Track, -1), "ASAP_PlaySong");
    }

    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      Sound::Chunk result(samples);
      const auto fmt = isLE() ? ASAPSampleFormat_S16_L_E : ASAPSampleFormat_S16_B_E;
      auto* const stereo = result.data();
      if (Channels == 2)
      {
        const auto bytes = static_cast<int>(samples * sizeof(*stereo));
        CheckError(bytes == ::ASAP_Generate(Module, safe_ptr_cast<unsigned char*>(stereo), bytes, fmt),
                   "ASAP_Generate");
      }
      else
      {
        auto* const mono = safe_ptr_cast<Sound::Sample::Type*>(stereo) + samples;
        const auto bytes = static_cast<int>(samples * sizeof(*mono));
        CheckError(bytes == ::ASAP_Generate(Module, safe_ptr_cast<unsigned char*>(mono), bytes, fmt), "ASAP_Generate");
        std::transform(mono, mono + samples, stereo, [](Sound::Sample::Type val) { return Sound::Sample(val, val); });
      }
      return result;
    }

    void Seek(Time::Instant<TimeUnit> request)
    {
      CheckError(::ASAP_Seek(Module, request.Get()), "ASAP_Seek");
    }

    void MuteChannels(int mask)
    {
      ::ASAP_MutePokeyChannels(Module, mask);
    }

  private:
    Strings::Array GetInstruments(Binary::View data) const
    {
      Strings::Array instruments;
      for (int idx = 0;; ++idx)
      {
        if (const auto* const ins = ::ASAPInfo_GetInstrumentName(Info, data.As<unsigned char>(), data.Size(), idx))
        {
          instruments.emplace_back(Strings::OptimizeAscii(ins));
        }
        else
        {
          break;
        }
      }
      return instruments;
    }

    Strings::Array GetChannels() const
    {
      constexpr auto CHANNELS_PER_CHIP = 4;
      const auto chipsCount = ::ASAPInfo_GetChannels(Info);
      Strings::Array channels;
      channels.reserve(chipsCount * CHANNELS_PER_CHIP);
      for (auto chip : xrange(chipsCount))
      {
        for (auto chan : xrange(CHANNELS_PER_CHIP))
        {
          // As in RasterMusicTracker
          const auto pfx = chipsCount == 1 ? "C"sv : (chip == 0 ? "L"sv : "R"sv);
          channels.emplace_back(pfx + std::to_string(chan + 1));
        }
      }
      return channels;
    }

    static void CheckError(bool ok, const char* msg)
    {
      if (!ok)
      {
        throw Error(THIS_LINE, msg);
      }
    }

  private:
    struct ASAP* const Module;
    const struct ASAPInfo* Info = nullptr;
    int Track;
    int Channels = 0;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  uint_t GetSamples(Time::Microseconds period)
  {
    return period.Get() * ASAP_SAMPLE_RATE / period.PER_SECOND;
  }

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(AsapTune::Ptr tune, Sound::Converter::Ptr target, Parameters::Accessor::Ptr params)
      : Tune(std::move(tune))
      , State(MakePtr<TimedState>(Tune->GetDuration()))
      , Target(std::move(target))
      , Params(std::move(params))
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      ApplyParameters();
      const auto avail = State->ConsumeUpTo(FRAME_DURATION);
      return Target->Apply(Tune->Render(GetSamples(avail)));
    }

    void Reset() override
    {
      try
      {
        State->Reset();
        Tune->Reset();
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      State->Seek(request);
      try
      {
        Tune->Seek(State->At());
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }

  private:
    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Core;
        const auto mask = Parameters::GetInteger(*Params, CHANNELS_MASK, CHANNELS_MASK_DEFAULT);
        Tune->MuteChannels(mask);
      }
    }

  private:
    const AsapTune::Ptr Tune;
    const TimedState::Ptr State;
    const Sound::Converter::Ptr Target;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(AsapTune::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return CreateTimedInfo(Tune->GetDuration());
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      try
      {
        return MakePtr<Renderer>(Tune, Sound::CreateResampler(ASAP_SAMPLE_RATE, samplerate), std::move(params));
      }
      catch (const std::exception& e)
      {
        throw Error(THIS_LINE, e.what());
      }
    }

  private:
    const AsapTune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  struct PluginDescription
  {
    const ZXTune::PluginId Id;
    const uint_t ChiptuneCaps;
  };

  using ZXTune::operator""_id;

  class MultitrackFactory : public Module::MultitrackFactory
  {
  public:
    explicit MultitrackFactory(PluginDescription desc)
      : Desc(std::move(desc))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params,
                                     const Formats::Multitrack::Container& container,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        auto tune = MakePtr<AsapTune>(Desc.Id, container, container.StartTrackIndex());

        PropertiesHelper props(*properties);
        props.SetPlatform(Platforms::ATARI);
        tune->GetProperties(container, props);

        tune->FillDuration(params);
        return MakePtr<Holder>(std::move(tune), std::move(properties));
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create {}: {}", Desc.Id, e.what());
      }
      return {};
    }

  private:
    const PluginDescription Desc;
  };

  struct MultitrackPluginDescription
  {
    using MultitrackDecoderCreator = Formats::Multitrack::Decoder::Ptr (*)();

    PluginDescription Desc;
    const MultitrackDecoderCreator CreateMultitrackDecoder;
  };

  // clang-format off
  const MultitrackPluginDescription MULTITRACK_PLUGINS[] =
  {
    {
      //sap
      {
        "SAP"_id,
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::CO12294,
      },
      &Formats::Multitrack::CreateSAPDecoder,
    }
  };
  // clang-format on

  class SingletrackFactory : public Module::ExternalParsingFactory
  {
  public:
    explicit SingletrackFactory(PluginDescription desc)
      : Desc(std::move(desc))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Formats::Chiptune::Container& container,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        auto tune = MakePtr<AsapTune>(Desc.Id, container, 0);

        Require(tune->GetSongsCount() == 1);

        PropertiesHelper props(*properties);
        props.SetPlatform(Platforms::ATARI);
        tune->GetProperties(container, props);

        tune->FillDuration(params);
        return MakePtr<Holder>(std::move(tune), std::move(properties));
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create {}: {}", Desc.Id, e.what());
      }
      return {};
    }

  private:
    const PluginDescription Desc;
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };

  struct SingletrackPluginDescription
  {
    using ChiptuneDecoderCreator = Formats::Chiptune::Decoder::Ptr (*)();

    PluginDescription Desc;
    const ChiptuneDecoderCreator CreateChiptuneDecoder;
  };

  // clang-format off
  const SingletrackPluginDescription SINGLETRACK_PLUGINS[] =
  {
    //rmt
    {
      {
        "RMT"_id,
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::CO12294,
      },
      &Formats::Chiptune::CreateRasterMusicTrackerDecoder
    },
  };
  // clang-format on
}  // namespace Module::ASAP

namespace ZXTune
{
  void RegisterASAPPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    for (const auto& desc : Module::ASAP::MULTITRACK_PLUGINS)
    {
      auto decoder = desc.CreateMultitrackDecoder();
      auto factory = MakePtr<Module::ASAP::MultitrackFactory>(desc.Desc);
      {
        auto plugin = CreatePlayerPlugin(desc.Desc.Id, desc.Desc.ChiptuneCaps, decoder, factory);
        players.RegisterPlugin(std::move(plugin));
      }
      {
        auto plugin = CreateArchivePlugin(desc.Desc.Id, std::move(decoder), std::move(factory));
        archives.RegisterPlugin(std::move(plugin));
      }
    }
    for (const auto& desc : Module::ASAP::SINGLETRACK_PLUGINS)
    {
      auto decoder = desc.CreateChiptuneDecoder();
      auto factory = MakePtr<Module::ASAP::SingletrackFactory>(desc.Desc);
      auto plugin = CreatePlayerPlugin(desc.Desc.Id, desc.Desc.ChiptuneCaps, std::move(decoder), std::move(factory));
      players.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
