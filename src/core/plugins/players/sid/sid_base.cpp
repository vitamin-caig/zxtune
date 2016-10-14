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
#include "roms.h"
#include "songlengths.h"
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
//3rdparty includes
#include <3rdparty/sidplayfp/sidplayfp/sidplayfp.h>
#include <3rdparty/sidplayfp/sidplayfp/SidInfo.h>
#include <3rdparty/sidplayfp/sidplayfp/SidTune.h>
#include <3rdparty/sidplayfp/sidplayfp/SidTuneInfo.h>
#include <3rdparty/sidplayfp/builders/resid-builder/resid.h>
//boost includes
#include <boost/algorithm/string/predicate.hpp>

namespace Module
{
namespace Sid
{
  const Debug::Stream Dbg("Core::SIDSupp");

  typedef std::shared_ptr<SidTune> TunePtr;
  typedef std::shared_ptr<sidplayfp> EnginePtr;

  void CheckSidplayError(bool ok)
  {
    Require(ok);//TODO
  }

  class Analyzer : public Module::Analyzer
  {
  public:
    typedef std::shared_ptr<Analyzer> Ptr;

    explicit Analyzer(EnginePtr engine)
      : Engine(engine)
    {
    }

    void SetClockRate(uint_t rate)
    {
      //Fout = (Fn * Fclk/16777216) Hz
      //http://www.waitingforfriday.com/index.php/Commodore_SID_6581_Datasheet
      Analysis.SetClockAndDivisor(rate, 16777216);
    }

    void GetState(std::vector<ChannelState>& channels) const override
    {
      unsigned freqs[6], levels[6];
      const unsigned count = Engine->getState(freqs, levels);
      std::vector<ChannelState> result(count);
      for (uint_t chan = 0; chan != count; ++chan)
      {
        ChannelState& res = result[chan];
        res.Band = Analysis.GetBandByScaledFrequency(freqs[chan]);
        res.Level = levels[chan] * 100 / 15;
      }
      channels.swap(result);
    }
  private:
    const EnginePtr Engine;
    Devices::Details::AnalysisMap Analysis;
  };

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
      : Params(params)
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

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(TunePtr tune, StateIterator::Ptr iterator, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Tune(tune)
      , Engine(std::make_shared<sidplayfp>())
      , Builder("resid")
      , Iterator(iterator)
      , State(Iterator->GetStateObserver())
      , Analysis(MakePtr<Analyzer>(Engine))
      , Target(target)
      , SoundParams(Sound::RenderParameters::Create(params))
      , Params(params)
      , Config(Engine->config())
      , UseFilter()
      , Looped()
      , SamplesPerFrame()
    {
      LoadRoms(*params);
      const uint_t chipsCount = Engine->info().maxsids();
      Builder.create(chipsCount);
      Config.frequency = 0;
      ApplyParameters();
      CheckSidplayError(Engine->load(tune.get()));
    }

    TrackState::Ptr GetTrackState() const override
    {
      return State;
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Analysis;
    }

    bool RenderFrame() override
    {
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");

      try
      {
        ApplyParameters();

        Sound::ChunkBuilder builder;
        builder.Reserve(SamplesPerFrame);
        Engine->play(safe_ptr_cast<short*>(builder.Allocate(SamplesPerFrame)), SamplesPerFrame * Sound::Sample::CHANNELS);
        Target->ApplyData(builder.GetResult());
        Iterator->NextFrame(Looped);
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
      Engine->stop();
      Iterator->Reset();
    }

    void SetPosition(uint_t frame) override
    {
      SeekEngine(frame);
      Module::SeekIterator(*Iterator, frame);
    }
  private:
    void LoadRoms(const Parameters::Accessor& params)
    {
      Parameters::DataType kernal, basic, chargen;
      params.FindValue(Parameters::ZXTune::Core::Plugins::SID::KERNAL, kernal);
      params.FindValue(Parameters::ZXTune::Core::Plugins::SID::BASIC, basic);
      params.FindValue(Parameters::ZXTune::Core::Plugins::SID::CHARGEN, chargen);
      Engine->setRoms(GetData(kernal, GetKernalROM()), GetData(basic, GetBasicROM()), GetData(chargen, GetChargenROM()));
    }

    static const uint8_t* GetData(const Parameters::DataType& dump, const uint8_t* defVal)
    {
      return dump.empty() ? defVal : &dump.front();
    }

    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        const uint_t newFreq = SoundParams->SoundFreq();
        const bool newFastSampling = Params.GetFastSampling();
        const SidConfig::sampling_method_t newSamplingMethod = Params.GetSamplingMethod();
        const bool newFilter = Params.GetUseFilter();
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
          CheckSidplayError(Engine->config(Config));
          Analysis->SetClockRate(Engine->getCPUFreq());
        }
        Looped = SoundParams->Looped();
        SamplesPerFrame = SoundParams->SamplesPerFrame();
      }
    }

    void SeekEngine(uint_t frame)
    {
      uint_t current = State->Frame();
      if (frame < current)
      {
        Engine->stop();
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        AdvanceEngine(delta);
      }
    }

    void AdvanceEngine(uint_t framesToPlay)
    {
      Engine->play(0, framesToPlay * SamplesPerFrame * Sound::Sample::CHANNELS);
    }
  private:
    const TunePtr Tune;
    const EnginePtr Engine;
    ReSIDBuilder Builder;
    const StateIterator::Ptr Iterator;
    const TrackState::Ptr State;
    const Analyzer::Ptr Analysis;
    const Sound::Receiver::Ptr Target;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const SidParameters Params;
    SidConfig Config;
    //cache filter flag
    bool UseFilter;
    bool Looped;
    std::size_t SamplesPerFrame;
  };

  class Information : public Module::Information
  {
  public:
    Information(const TimeType defaultDuration, TunePtr tune, uint_t fps, uint_t songIdx)
      : DefaultDuration(defaultDuration)
      , Tune(tune)
      , Fps(fps)
      , SongIdx(songIdx)
      , Frames()
    {
    }

    uint_t PositionsCount() const override
    {
      return 1;
    }

    uint_t LoopPosition() const override
    {
      return 0;
    }

    uint_t PatternsCount() const override
    {
      return 0;
    }

    uint_t FramesCount() const override
    {
      if (!Frames)
      {
        Frames = GetFramesCount();
      }
      return Frames;
    }

    uint_t LoopFrame() const override
    {
      return 0;
    }

    uint_t ChannelsCount() const override
    {
      return 1;
    }

    uint_t Tempo() const override
    {
      return 1;
    }
  private:
    uint_t GetFramesCount() const
    {
      const char* md5 = Tune->createMD5();
      const TimeType knownDuration = GetSongLength(md5, SongIdx - 1);
      const TimeType duration = knownDuration == TimeType() ? DefaultDuration : knownDuration;
      Dbg("Duration for %1%/%2% is %3%ms", md5, SongIdx, duration.Get());
      return Fps * (duration.Get() / duration.PER_SECOND);
    }
  private:
    const TimeType DefaultDuration;
    const TunePtr Tune;
    const uint_t Fps;
    const uint_t SongIdx;
    mutable uint_t Frames;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(TunePtr tune, Information::Ptr info, Parameters::Accessor::Ptr props)
      : Tune(tune)
      , Info(info)
      , Properties(props)
    {
    }

    Module::Information::Ptr GetModuleInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return MakePtr<Renderer>(Tune, Module::CreateStreamStateIterator(Info), target, params);
    }
  private:
    const TunePtr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  bool HasSidContainer(const Parameters::Accessor& params)
  {
    Parameters::StringType container;
    Require(params.FindValue(Module::ATTR_CONTAINER, container));
    return container == "SID" || boost::algorithm::ends_with(container, ">SID");
  }

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        const TunePtr tune = std::make_shared<SidTune>(static_cast<const uint_least8_t*>(rawData.Start()),
          static_cast<uint_least32_t>(rawData.Size()));
        CheckSidplayError(tune->getStatus());
        const unsigned songIdx = tune->selectSong(0);

        const SidTuneInfo& tuneInfo = *tune->getInfo();
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
          props.SetComment(FromStdString(tuneInfo.infoString(2)));
        case 2:
          props.SetAuthor(FromStdString(tuneInfo.infoString(1)));
        case 1:
          props.SetTitle(FromStdString(tuneInfo.infoString(0)));
        case 0:
          break;
        }
        const Binary::Container::Ptr data = rawData.GetSubcontainer(0, tuneInfo.dataFileLen());
        const Formats::Chiptune::Container::Ptr source = Formats::Chiptune::CreateCalculatingCrcContainer(data, 0, data->Size());
        props.SetSource(*source);

        const uint_t fps = tuneInfo.songSpeed() == SidTuneInfo::SPEED_CIA_1A || tuneInfo.clockSpeed() == SidTuneInfo::CLOCK_NTSC ? 60 : 50;
        props.SetFramesFrequency(fps);

        const Information::Ptr info = MakePtr<Information>(GetDuration(params), tune, fps, songIdx);
        return MakePtr<Holder>(tune, info, properties);
      }
      catch (const std::exception&)
      {
        return Holder::Ptr();
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
