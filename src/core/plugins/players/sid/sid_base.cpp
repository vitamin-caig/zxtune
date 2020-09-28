/**
* 
* @file
*
* @brief  SID support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/sid/roms.h"
#include "core/plugins/players/sid/songlengths.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <core/core_parameters.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <debug/log.h>
#include <devices/details/analysis_map.h>
#include <formats/chiptune/container.h>
#include <formats/chiptune/emulation/sid.h>
#include <module/attributes.h>
#include <module/players/duration.h>
#include <module/players/properties_helper.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
#include <strings/encoding.h>
#include <strings/trim.h>
//3rdparty includes
#include <3rdparty/sidplayfp/sidplayfp/sidplayfp.h>
#include <3rdparty/sidplayfp/sidplayfp/SidInfo.h>
#include <3rdparty/sidplayfp/sidplayfp/SidTune.h>
#include <3rdparty/sidplayfp/sidplayfp/SidTuneInfo.h>
#include <3rdparty/sidplayfp/builders/resid-builder/resid.h>
//boost includes
#include <boost/algorithm/string/predicate.hpp>
//text includes
#include <module/text/platforms.h>

namespace Module
{
namespace Sid
{
  const Debug::Stream Dbg("Core::SIDSupp");

  void CheckSidplayError(bool ok)
  {
    Require(ok);//TODO
  }

  class Model : public SidTune
  {
  public:
    using Ptr = std::shared_ptr<Model>;

    explicit Model(Binary::View data)
      : SidTune(static_cast<const uint_least8_t*>(data.Start()), data.Size())
      , Index(selectSong(0))
    {
      CheckSidplayError(getStatus());
    }

    FramedStream CreateStream(const Parameters::Accessor& params)/*const*/
    {
      const auto& tuneInfo = *getInfo();
      const uint_t fps = tuneInfo.songSpeed() == SidTuneInfo::SPEED_CIA_1A || tuneInfo.clockSpeed() == SidTuneInfo::CLOCK_NTSC ? 60 : 50;
      const auto* md5 = createMD5();
      auto duration = GetSongLength(md5, Index - 1);
      if (!duration.Get())
      {
        duration = GetDefaultDuration(params);
      }
      Dbg("Duration for %1%/%2% is %3%ms", md5, Index, duration.Get());

      FramedStream result;
      result.FrameDuration = Time::Microseconds::FromFrequency(fps);
      result.TotalFrames = duration.Divide<uint_t>(result.FrameDuration);
      return result;
    }
  private:
    uint_t Index = 0;
  };

  inline const uint8_t* GetData(const Parameters::DataType& dump, const uint8_t* defVal)
  {
    return dump.empty() ? defVal : dump.data();
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
    explicit SidParameters(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {
    }

    bool GetFastSampling() const
    {
      return Parameters::ZXTune::Core::SID::INTERPOLATION_NONE == GetInterpolation();
    }

    SidConfig::sampling_method_t GetSamplingMethod() const
    {
      return Parameters::ZXTune::Core::SID::INTERPOLATION_HQ == GetInterpolation()
          ? SidConfig::RESAMPLE_INTERPOLATE : SidConfig::INTERPOLATE;
    }

    bool GetUseFilter() const
    {
      Parameters::IntType val = Parameters::ZXTune::Core::SID::FILTER_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Core::SID::FILTER, val);
      return static_cast<bool>(val);
    }
  private:
    Parameters::IntType GetInterpolation() const
    {
      Parameters::IntType val = Parameters::ZXTune::Core::SID::INTERPOLATION_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Core::SID::INTERPOLATION, val);
      return val;
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  class SidEngine : public Module::Analyzer
  {
  public:
    using Ptr = std::shared_ptr<SidEngine>;

    SidEngine()
      : Builder("resid")
      , Config(Player.config())
      , UseFilter()
    {
    }

    void Init(const Parameters::Accessor& params)
    {
      Parameters::DataType kernal, basic, chargen;
      params.FindValue(Parameters::ZXTune::Core::Plugins::SID::KERNAL, kernal);
      params.FindValue(Parameters::ZXTune::Core::Plugins::SID::BASIC, basic);
      params.FindValue(Parameters::ZXTune::Core::Plugins::SID::CHARGEN, chargen);
      Player.setRoms(GetData(kernal, GetKernalROM()), GetData(basic, GetBasicROM()), GetData(chargen, GetChargenROM()));
      const uint_t chipsCount = Player.info().maxsids();
      Builder.create(chipsCount);
      Config.frequency = 0;
    }

    void Load(SidTune& tune)
    {
      CheckSidplayError(Player.load(&tune));
    }

    void ApplyParameters(const Sound::RenderParameters& soundParams, const SidParameters& sidParams)
    {
      const auto newFreq = soundParams.SoundFreq();
      const auto newFastSampling = sidParams.GetFastSampling();
      const auto newSamplingMethod = sidParams.GetSamplingMethod();
      const auto newFilter = sidParams.GetUseFilter();
      if (Config.frequency != newFreq
          || Config.fastSampling != newFastSampling
          || Config.samplingMethod != newSamplingMethod
          || UseFilter != newFilter)
      {
        Config.frequency = newFreq;
        Config.playback = Sound::Sample::CHANNELS == 1 ? SidConfig::MONO : SidConfig::STEREO;

        Config.fastSampling = newFastSampling;
        Config.samplingMethod = newSamplingMethod;
        Builder.filter(UseFilter = newFilter);

        Config.sidEmulation = &Builder;
        CheckSidplayError(Player.config(Config));
        SetClockRate(Player.getCPUFreq());
      }
    }

    void Stop()
    {
      Player.stop();
    }

    void Render(short* target, uint_t samples)
    {
      Player.play(target, samples);
    }

    SpectrumState GetState() const override
    {
      unsigned freqs[6], levels[6];
      const auto count = Player.getState(freqs, levels);
      SpectrumState result;
      for (uint_t chan = 0; chan != count; ++chan)
      {
        const auto band = Analysis.GetBandByScaledFrequency(freqs[chan]);
        result.Set(band, LevelType(levels[chan], 15));
      }
      return result;
    }
  private:
    void SetClockRate(uint_t rate)
    {
      //Fout = (Fn * Fclk/16777216) Hz
      //http://www.waitingforfriday.com/index.php/Commodore_SID_6581_Datasheet
      Analysis.SetClockAndDivisor(rate, 16777216);
    }

  private:
    sidplayfp Player;
    ReSIDBuilder Builder;
    SidConfig Config;
    
    //cache filter flag
    bool UseFilter;
    Devices::Details::AnalysisMap Analysis;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr tune, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Tune(std::move(tune))
      , Stream(Tune->CreateStream(*params))
      , Engine(MakePtr<SidEngine>())
      , Iterator(CreateStreamStateIterator(Stream))
      , Target(std::move(target))
      , Params(params)
      , SoundParams(Sound::RenderParameters::Create(params))
    {
      Engine->Init(*params);
      ApplyParameters();
      Engine->Load(*Tune);
    }

    State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Engine;
    }

    bool RenderFrame(const Sound::LoopParameters& looped) override
    {
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");

      try
      {
        ApplyParameters();

        Sound::ChunkBuilder builder;
        builder.Reserve(SamplesPerFrame);
        Engine->Render(safe_ptr_cast<short*>(builder.Allocate(SamplesPerFrame)), SamplesPerFrame * Sound::Sample::CHANNELS);
        Target->ApplyData(builder.CaptureResult());
        Iterator->NextFrame(looped);
        return Iterator->IsValid();
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      SoundParams.Reset();
      Engine->Stop();
      Iterator->Reset();
    }

    void SetPosition(uint_t frame) override
    {
      SeekEngine(frame);
      Module::SeekIterator(*Iterator, frame);
    }
  private:

    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Engine->ApplyParameters(*SoundParams, Params);
        //TODO: simplify
        SamplesPerFrame = Stream.FrameDuration.Get() * SoundParams->SoundFreq() / Stream.FrameDuration.PER_SECOND;
      }
    }

    void SeekEngine(uint_t frame)
    {
      uint_t current = GetState()->Frame();
      if (frame < current)
      {
        Engine->Stop();
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        Engine->Render(nullptr, delta * SamplesPerFrame * Sound::Sample::CHANNELS);
      }
    }
  private:
    const Model::Ptr Tune;
    const FramedStream Stream;
    const SidEngine::Ptr Engine;
    const StateIterator::Ptr Iterator;
    const Sound::Receiver::Ptr Target;
    const SidParameters Params;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    std::size_t SamplesPerFrame = 0;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr tune, FramedStream stream, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Stream(std::move(stream))
      , Properties(std::move(props))
    {
    }

    Module::Information::Ptr GetModuleInformation() const override
    {
      return CreateStreamInfo(Stream);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return MakePtr<Renderer>(Tune, std::move(target), std::move(params));
    }
  private:
    const Model::Ptr Tune;
    const FramedStream Stream;
    const Parameters::Accessor::Ptr Properties;
  };

  bool HasSidContainer(const Parameters::Accessor& params)
  {
    Parameters::StringType container;
    Require(params.FindValue(Module::ATTR_CONTAINER, container));
    return container == "SID" || boost::algorithm::ends_with(container, ">SID");
  }
  
  String DecodeString(StringView str)
  {
    return Strings::ToAutoUtf8(Strings::TrimSpaces(str));
  }

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        auto tune = MakePtr<Model>(rawData);

        const auto& tuneInfo = *tune->getInfo();
        if (tuneInfo.songs() > 1)
        {
          Require(HasSidContainer(*properties));
        }

        PropertiesHelper props(*properties);
        switch (tuneInfo.numberOfInfoStrings())
        {
        default:
        case 3:
          //copyright/publisher really
          props.SetComment(DecodeString(tuneInfo.infoString(2)));
        case 2:
          props.SetAuthor(DecodeString(tuneInfo.infoString(1)));
        case 1:
          props.SetTitle(DecodeString(tuneInfo.infoString(0)));
        case 0:
          break;
        }
        auto data = rawData.GetSubcontainer(0, tuneInfo.dataFileLen());
        props.SetSource(*Formats::Chiptune::CreateCalculatingCrcContainer(std::move(data), 0, data->Size()));

        props.SetPlatform(Platforms::COMMODORE_64);

        auto stream = tune->CreateStream(params);
        return MakePtr<Holder>(std::move(tune), std::move(stream), std::move(properties));
      }
      catch (const std::exception&)
      {
        return {};
      }
    }
  };
}
}

namespace ZXTune
{
  void RegisterSIDPlugins(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'S', 'I', 'D', 0};
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::MOS6581;
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSIDDecoder();
    const Module::Factory::Ptr factory = MakePtr<Module::Sid::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
