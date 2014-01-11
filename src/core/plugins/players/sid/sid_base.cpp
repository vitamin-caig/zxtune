/**
* 
* @file
*
* @brief  SID support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <contract.h>
//local includes
#include "core/plugins/registrator.h"
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/streaming.h"
//library includes
#include <binary/format_factories.h>
#include <core/core_parameters.h>
#include <core/plugin_attrs.h>
#include <devices/details/parameters_helper.h>
#include <formats/chiptune/container.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
#include <time/stamp.h>
//3rdparty includes
#include <3rdparty/sidplayfp/sidplayfp/sidplayfp.h>
#include <3rdparty/sidplayfp/sidplayfp/SidInfo.h>
#include <3rdparty/sidplayfp/sidplayfp/SidTune.h>
#include <3rdparty/sidplayfp/sidplayfp/SidTuneInfo.h>
#include <3rdparty/sidplayfp/builders/resid/resid.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/range/end.hpp>

namespace Module
{
namespace Sid
{
  typedef Time::Milliseconds TimeType;

  typedef boost::shared_ptr<SidTune> TunePtr;

  void CheckSidplayError(bool ok)
  {
    Require(ok);//TODO
  }

  class Analyzer : public Module::Analyzer
  {
  public:
    virtual void GetState(std::vector<ChannelState>& channels) const
    {
      channels.clear();;
    }

    static Ptr Create()
    {
      static Analyzer instance;
      return MakeSingletonPointer(instance);
    }
  };

  /*
   * Interpolation modes
   * 0 - fast sampling+interpolate
   * 1 - regular sampling+interpolate
   * 2 - regular sampling+interpolate+resample
   */

  enum InterpolationModes
  {
    FAST_SAMPLING_INTERPOLATE,
    INTERPOLATE,
    RESAMPLE_INTERPOLATE
  };

  class SidParameters
  {
  public:
    explicit SidParameters(Parameters::Accessor::Ptr params)
      : Params(params)
    {
    }

    bool GetFastSampling() const
    {
      return FAST_SAMPLING_INTERPOLATE == GetInterpolation();
    }

    SidConfig::sampling_method_t GetSamplingMethod() const
    {
      return RESAMPLE_INTERPOLATE == GetInterpolation()
          ? SidConfig::RESAMPLE_INTERPOLATE : SidConfig::INTERPOLATE;
    }

    bool GetUseFilter() const
    {
      return GetBool(Parameters::ZXTune::Core::SID::FILTER);
    }
  private:
    Parameters::IntType GetInterpolation() const
    {
      Parameters::IntType val = 0;
      Params->FindValue(Parameters::ZXTune::Core::SID::INTERPOLATION, val);
      return val;
    }

    bool GetBool(const Parameters::NameType& name) const
    {
      Parameters::IntType val = 0;
      Params->FindValue(name, val);
      return val != 0;
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(TunePtr tune, StateIterator::Ptr iterator, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Tune(tune)
      , Builder("resid")
      , Iterator(iterator)
      , State(Iterator->GetStateObserver())
      , Target(target)
      , SoundParams(Sound::RenderParameters::Create(params))
      , Params(params)
      , Config(Engine.config())
      , UseFilter()
      , Looped()
      , SamplesPerFrame()
    {
      LoadRoms(*params);
      const uint_t chipsCount = Engine.info().maxsids();
      Builder.create(chipsCount);
      Config.frequency = 0;
      ApplyParameters();
      CheckSidplayError(Engine.load(tune.get()));
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return State;
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return Analyzer::Create();
    }

    virtual bool RenderFrame()
    {
      BOOST_STATIC_ASSERT(Sound::Sample::BITS == 16);

      try
      {
        ApplyParameters();

        Sound::ChunkBuilder builder;
        builder.Reserve(SamplesPerFrame);
        Engine.play(safe_ptr_cast<short*>(builder.Allocate(SamplesPerFrame)), SamplesPerFrame * Sound::Sample::CHANNELS);
        Target->ApplyData(builder.GetResult());
        Iterator->NextFrame(Looped);
        return Iterator->IsValid();
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    virtual void Reset()
    {
      Engine.stop();
      Iterator->Reset();
    }

    virtual void SetPosition(uint_t frame)
    {
      SeekEngine(frame);
      Module::SeekIterator(*Iterator, frame);
    }
  private:
    void LoadRoms(const Parameters::Accessor& params)
    {
      Parameters::DataType kernal, basic, chargen;
      params.FindValue(Parameters::ZXTune::Core::SID::ROM::KERNAL, kernal);
      params.FindValue(Parameters::ZXTune::Core::SID::ROM::BASIC, basic);
      params.FindValue(Parameters::ZXTune::Core::SID::ROM::CHARGEN, chargen);
      if (!kernal.empty() || !basic.empty() || !chargen.empty())
      {
        Engine.setRoms(GetData(kernal), GetData(basic), GetData(chargen));
      }
    }

    static const uint8_t* GetData(const Parameters::DataType& dump)
    {
      return dump.empty() ? 0 : &dump.front();
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
          CheckSidplayError(Engine.config(Config));
          //config() causes playback restart
          Iterator->Reset();
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
        Engine.stop();
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        AdvanceEngine(delta);
      }
    }

    void AdvanceEngine(uint_t framesToPlay)
    {
      std::vector<short> fakeBuf(SamplesPerFrame * Sound::Sample::CHANNELS);
      uint_t restFrames = framesToPlay;
      if (restFrames >= 32)
      {
        restFrames = AdvanceEngine(restFrames, 32, fakeBuf);
      }
      AdvanceEngine(restFrames, 1, fakeBuf);
    }

    uint_t AdvanceEngine(uint_t frames, uint_t scale, std::vector<short>& buf)
    {
      Engine.fastForward(scale * 100);
      while (frames > scale)
      {
        Engine.play(&buf.front(), buf.size());
        frames -= scale;
      }
      return frames;
    }
  private:
    const TunePtr Tune;
    sidplayfp Engine;
    ReSIDBuilder Builder;
    const StateIterator::Ptr Iterator;
    const TrackState::Ptr State;
    const Sound::Receiver::Ptr Target;
    const Devices::Details::ParametersHelper<Sound::RenderParameters> SoundParams;
    const SidParameters Params;
    SidConfig Config;
    //cache filter flag
    bool UseFilter;
    bool Looped;
    std::size_t SamplesPerFrame;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(TunePtr tune, Parameters::Accessor::Ptr props, uint_t frames)
      : Tune(tune)
      , Info(Module::CreateStreamInfo(frames, 0))
      , Properties(props)
    {
    }

    virtual Module::Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Properties;
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      return boost::make_shared<Renderer>(Tune, Module::CreateStreamStateIterator(Info), target, params);
    }
  private:
    const TunePtr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  class Format : public Binary::Format
  {
  public:
    virtual bool Match(const Binary::Data& /*data*/) const
    {
      return true;
    }

    virtual std::size_t NextMatchOffset(const Binary::Data& data) const
    {
      return data.Size();
    }
  };

  struct PluginDescription
  {
    const char* const Id;
    const Char* const Description;
    const char* const Format;
  };

  class Decoder : public Formats::Chiptune::Decoder
  {
  public:
    explicit Decoder(const PluginDescription& desc)
      : Desc(desc)
      , Fmt(Desc.Format ? Binary::CreateMatchOnlyFormat(Desc.Format) : boost::make_shared<Format>())
    {
    }

    virtual String GetDescription() const
    {
      return Desc.Description;
    }


    virtual Binary::Format::Ptr GetFormat() const
    {
      return Fmt;
    }

    virtual bool Check(const Binary::Container& rawData) const
    {
      return Fmt->Match(rawData);
    }

    virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
    {
      return Formats::Chiptune::Container::Ptr();//TODO
    }
  private:
    const PluginDescription& Desc;
    const Binary::Format::Ptr Fmt;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(const PluginDescription& desc)
      : Desc(desc)
    {
    }

    virtual Module::Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      try
      {
        const TunePtr tune = boost::make_shared<SidTune>(static_cast<const uint_least8_t*>(rawData.Start()),
          static_cast<uint_least32_t>(rawData.Size()));
        CheckSidplayError(tune->getStatus());
        tune->selectSong(0);

        const SidTuneInfo& info = *tune->getInfo();

        switch (info.numberOfInfoStrings())
        {
        default:
        case 3:
          //copyright/publisher really
          propBuilder.SetComment(FromStdString(info.infoString(2)));
        case 2:
          propBuilder.SetAuthor(FromStdString(info.infoString(1)));
        case 1:
          propBuilder.SetTitle(FromStdString(info.infoString(0)));
        case 0:
          break;
        }
        const uint_t fps = info.songSpeed() == SidTuneInfo::SPEED_CIA_1A || info.clockSpeed() == SidTuneInfo::CLOCK_NTSC ? 60 : 50;
        propBuilder.SetValue(Parameters::ZXTune::Sound::FRAMEDURATION, Time::GetPeriodForFrequency<Time::Microseconds>(fps).Get());

        const Binary::Container::Ptr data = rawData.GetSubcontainer(0, info.dataFileLen());
        const Formats::Chiptune::Container::Ptr source = Formats::Chiptune::CreateCalculatingCrcContainer(data, 0, data->Size());
        propBuilder.SetSource(*source);

        const uint_t frames = 10000;//TODO
        return boost::make_shared<Holder>(tune, propBuilder.GetResult(), frames);
      }
      catch (const std::exception&)
      {
        return Holder::Ptr();
      }
    }
  private:
    const PluginDescription& Desc;
  };

  const PluginDescription PLUGINS[] =
  {
    //RSID/PSID
    {
      "SID"
      ,
      "C64 SID"      //TODO
      ,
      "'R|'P 'S'I'D" //signature
      "00 01|02"     //BE version
      "00 76|7c"     //BE data offset
      "??"           //BE load address
      "??"           //BE init address
      "??"           //BE play address
      "00|01 ?"      //BE songs count 1-256
      "??"           //BE start song
      "????"         //BE speed flag
    },
    //TODO: extract PowerPacker container
    //MUS files are not supported due to lack of structure
  };
}
}

namespace ZXTune
{
  void RegisterSIDPlugins(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = CAP_DEV_MOS6581 | CAP_STOR_MODULE | CAP_CONV_RAW;
    for (const Module::Sid::PluginDescription* it = Module::Sid::PLUGINS; it != boost::end(Module::Sid::PLUGINS); ++it)
    {
      const Module::Sid::PluginDescription& desc = *it;

      const Formats::Chiptune::Decoder::Ptr decoder = boost::make_shared<Module::Sid::Decoder>(desc);
      const Module::Factory::Ptr factory = boost::make_shared<Module::Sid::Factory>(desc);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(FromStdString(desc.Id), CAPS, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
