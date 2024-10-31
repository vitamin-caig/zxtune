/**
 *
 * @file
 *
 * @brief  SID support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "core/plugins/players/sid/roms.h"
#include "core/plugins/players/sid/songlengths.h"
#include <formats/multitrack/decoders.h>
#include <module/players/duration.h>
#include <module/players/platforms.h>
#include <module/players/properties_helper.h>
#include <module/players/streaming.h>

#include <core/core_parameters.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <debug/log.h>
#include <module/attributes.h>
#include <parameters/tracking_helper.h>
#include <strings/sanitize.h>

#include <contract.h>
#include <make_ptr.h>

#include <3rdparty/sidplayfp/src/builders/resid-builder/resid.h>
#include <3rdparty/sidplayfp/src/config.h>
#include <3rdparty/sidplayfp/src/sidmd5.h>
#include <3rdparty/sidplayfp/src/sidplayfp/SidInfo.h>
#include <3rdparty/sidplayfp/src/sidplayfp/SidTune.h>
#include <3rdparty/sidplayfp/src/sidplayfp/SidTuneInfo.h>
#include <3rdparty/sidplayfp/src/sidplayfp/sidplayfp.h>

namespace Module::Sid
{
  const Debug::Stream Dbg("Core::SIDSupp");

  void CheckSidplayError(bool ok)
  {
    Require(ok);  // TODO
  }

  class Model : public SidTune
  {
  public:
    using Ptr = std::shared_ptr<Model>;

    Model(Binary::View data, uint_t idx)
      : SidTune(static_cast<const uint_least8_t*>(data.Start()), data.Size())
      , Index(selectSong(idx + 1))
    {
      CheckSidplayError(getStatus());
      libsidplayfp::sidmd5 md5;
      md5.append(data.Start(), data.Size());
      md5.finish();
      MD5 = md5.getDigest();
    }

    void FillDuration(const Parameters::Accessor& params)
    {
      Duration = GetSongLength(MD5, Index - 1);
      if (!Duration)
      {
        Duration = GetDefaultDuration(params);
      }
      Dbg("Duration for {}/{} is {}ms", MD5, Index, Duration.Get());
    }

    Time::Milliseconds GetDuration() const
    {
      return Duration;
    }

  private:
    uint_t Index = 0;
    Time::Milliseconds Duration;
    std::string MD5;
  };

  inline const uint8_t* GetData(const Binary::Data::Ptr& data, const uint8_t* defVal)
  {
    return !data || !data->Size() ? defVal : static_cast<const uint8_t*>(data->Start());
  }

  /*
   * Interpolation modes
   * 0 - fast sampling+interpolate
   * 1 - regular sampling+interpolate
   * 2 - regular sampling+interpolate+resample
   */

  class SidParameters
  {
  public:
    using Ptr = std::unique_ptr<const SidParameters>;

    explicit SidParameters(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    uint_t Version() const
    {
      return Params->Version();
    }

    bool GetFastSampling() const
    {
      return Parameters::ZXTune::Core::SID::INTERPOLATION_NONE == GetInterpolation();
    }

    SidConfig::sampling_method_t GetSamplingMethod() const
    {
      return Parameters::ZXTune::Core::SID::INTERPOLATION_HQ == GetInterpolation() ? SidConfig::RESAMPLE_INTERPOLATE
                                                                                   : SidConfig::INTERPOLATE;
    }

    bool GetUseFilter() const
    {
      using namespace Parameters::ZXTune::Core::SID;
      return 0 != Parameters::GetInteger(*Params, FILTER, FILTER_DEFAULT);
    }

  private:
    Parameters::IntType GetInterpolation() const
    {
      using namespace Parameters::ZXTune::Core::SID;
      return Parameters::GetInteger(*Params, INTERPOLATION, INTERPOLATION_DEFAULT);
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  class SidEngine
  {
  public:
    using Ptr = std::unique_ptr<SidEngine>;

    SidEngine()
      : Builder("resid")
      , Config(Player.config())
    {}

    void Init(uint_t samplerate, const Parameters::Accessor& params)
    {
      const auto kernal = params.FindData(Parameters::ZXTune::Core::Plugins::SID::KERNAL);
      const auto basic = params.FindData(Parameters::ZXTune::Core::Plugins::SID::BASIC);
      const auto chargen = params.FindData(Parameters::ZXTune::Core::Plugins::SID::CHARGEN);
      Player.setRoms(GetData(kernal, GetKernalROM()), GetData(basic, GetBasicROM()), GetData(chargen, GetChargenROM()));
      const uint_t chipsCount = Player.info().maxsids();
      Builder.create(chipsCount);
      Config.frequency = samplerate;
      Config.powerOnDelay = SidConfig::MAX_POWER_ON_DELAY - 1;
    }

    void Load(SidTune& tune)
    {
      CheckSidplayError(Player.load(&tune));
    }

    void ApplyParameters(const SidParameters& sidParams)
    {
      const auto newFastSampling = sidParams.GetFastSampling();
      const auto newSamplingMethod = sidParams.GetSamplingMethod();
      const auto newFilter = sidParams.GetUseFilter();
      if (Config.fastSampling != newFastSampling || Config.samplingMethod != newSamplingMethod
          || UseFilter != newFilter)
      {
        Config.playback = Sound::Sample::CHANNELS == 1 ? SidConfig::MONO : SidConfig::STEREO;

        Config.fastSampling = newFastSampling;
        Config.samplingMethod = newSamplingMethod;
        Builder.filter(UseFilter = newFilter);
        Builder.bias(0.);

        Config.sidEmulation = &Builder;
        CheckSidplayError(Player.config(Config));
      }
    }

    uint_t GetSoundFreq() const
    {
      return Config.frequency;
    }

    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      Sound::Chunk result(samples);
      Player.play(safe_ptr_cast<short*>(result.data()), samples * Sound::Sample::CHANNELS);
      return result;
    }

    void Skip(uint_t samples)
    {
      Player.play(nullptr, samples * Sound::Sample::CHANNELS);
    }

  private:
    sidplayfp Player;
    ReSIDBuilder Builder;
    SidConfig Config;

    // cache filter flag
    bool UseFilter = false;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr tune, uint_t samplerate, const Parameters::Accessor::Ptr& params)
      : Tune(std::move(tune))
      , State(MakePtr<TimedState>(Tune->GetDuration()))
      , Engine(MakePtr<SidEngine>())
      , SidParams(MakePtr<SidParameters>(params))
    {
      Engine->Init(samplerate, *params);
      ApplyParameters();
      Reset();
    }

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      ApplyParameters();
      const auto avail = State->ConsumeUpTo(FRAME_DURATION);
      return Engine->Render(GetSamples(avail));
    }

    void Reset() override
    {
      State->Reset();
      Engine->Load(*Tune);
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      if (request < State->At())
      {
        Engine->Load(*Tune);
      }
      if (const auto toSkip = State->Seek(request))
      {
        Engine->Skip(GetSamples(toSkip));
      }
    }

  private:
    uint_t GetSamples(Time::Microseconds period) const
    {
      return period.Get() * Engine->GetSoundFreq() / period.PER_SECOND;
    }

    void ApplyParameters()
    {
      if (SidParams.IsChanged())
      {
        Engine->ApplyParameters(*SidParams);
      }
    }

  private:
    const Model::Ptr Tune;
    const TimedState::Ptr State;
    const SidEngine::Ptr Engine;
    const StateIterator::Ptr Iterator;
    Parameters::TrackingHelper<SidParameters> SidParams;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr tune, Parameters::Accessor::Ptr props)
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
      return MakePtr<Renderer>(Tune, samplerate, std::move(params));
    }

  private:
    const Model::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  class Factory : public Module::MultitrackFactory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params,
                                     const Formats::Multitrack::Container& container,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        auto tune = MakePtr<Model>(container, container.StartTrackIndex());

        const auto& tuneInfo = *tune->getInfo();
        Require(container.TracksCount() == tuneInfo.songs());

        PropertiesHelper props(*properties);
        switch (tuneInfo.numberOfInfoStrings())
        {
        default:
        case 3:
          // copyright/publisher really
          props.SetComment(Strings::SanitizeMultiline(tuneInfo.infoString(2)));
          [[fallthrough]];
        case 2:
          props.SetAuthor(Strings::Sanitize(tuneInfo.infoString(1)));
          [[fallthrough]];
        case 1:
          props.SetTitle(Strings::Sanitize(tuneInfo.infoString(0)));
          [[fallthrough]];
        case 0:
          break;
        }

        props.SetPlatform(Platforms::COMMODORE_64);

        tune->FillDuration(params);
        return MakePtr<Holder>(std::move(tune), std::move(properties));
      }
      catch (const std::exception&)
      {
        return {};
      }
    }
  };
}  // namespace Module::Sid

namespace ZXTune
{
  void RegisterSIDPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    const auto ID = "SID"_id;
    auto decoder = Formats::Multitrack::CreateSIDDecoder();
    auto factory = MakePtr<Module::Sid::Factory>();
    {
      const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::MOS6581;
      auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
      players.RegisterPlugin(std::move(plugin));
    }
    {
      auto plugin = CreateArchivePlugin(ID, std::move(decoder), std::move(factory));
      archives.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
