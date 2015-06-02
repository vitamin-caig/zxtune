/**
* 
* @file
*
* @brief  Game Music Emu-based formats support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <contract.h>
//library includes
#include <binary/format_factories.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <devices/details/analysis_map.h>
#include <formats/chiptune/container.h>
#include <formats/chiptune/emulation/nsf.h>
#include <math/numeric.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/range/end.hpp>
#include <boost/algorithm/string/predicate.hpp>
//3rdparty
#include <3rdparty/gme/gme/Nsfe_Emu.h>

namespace
{
  const Debug::Stream Dbg("Core::GMESupp");
}

namespace Module
{
namespace GME
{
  typedef boost::shared_ptr< ::Music_Emu> EmuPtr;

  typedef EmuPtr (*EmuCreator)();
  
  template<class EmuType>
  EmuPtr Create()
  {
    return boost::make_shared<EmuType>();
  }
  
  class GME : public Module::Analyzer
  {
  public:
    typedef boost::shared_ptr<GME> Ptr;
    
    GME(EmuCreator create, Binary::Data::Ptr data, uint_t soundFreq)
      : CreateEmu(create)
      , Data(data)
      , SoundFreq(0)
      , Track(0)
    {
      Load(soundFreq);
    }
    
    uint_t GetTotalTracks() const
    {
      return Emu->track_count();
    }
    
    void SetTrack(uint_t track)
    {
      Track = track;
    }
    
    void GetInfo(::track_info_t& info) const
    {
      CheckError(Emu->track_info(&info, Track));
    }
    
    void Reset()
    {
      CheckError(Emu->start_track(Track));
    }
    
    void Render(uint_t samples, Sound::ChunkBuilder& target)
    {
      BOOST_STATIC_ASSERT(Sound::Sample::CHANNELS == 2);
      BOOST_STATIC_ASSERT(Sound::Sample::BITS == 16);
      ::Music_Emu::sample_t* const buffer = safe_ptr_cast< ::Music_Emu::sample_t*>(target.Allocate(samples));
      CheckError(Emu->play(samples * Sound::Sample::CHANNELS, buffer));
    }
    
    void Skip(uint_t samples)
    {
      CheckError(Emu->skip(samples));
    }
    
    void SetSoundFreq(uint_t soundFreq)
    {
      if (soundFreq != SoundFreq)
      {
        Load(soundFreq);
      }
    }
    
    virtual void GetState(std::vector<ChannelState>& channels) const
    {
      std::vector<voice_status_t> voices(Emu->voice_count());
      const int actual = Emu->voices_status(&voices[0], voices.size());
      std::vector<ChannelState> result;
      for (int chan = 0; chan < actual; ++chan)
      {
        const voice_status_t& in = voices[chan];
        Analysis.SetClockRate(in.frequency);
        ChannelState state;
        state.Level = in.level * 100 / voice_max_level;
        state.Band = Analysis.GetBandByPeriod(in.divider);
        result.push_back(state);
      }
      channels.swap(result);
    }
  private:
    inline static void CheckError(::blargg_err_t err)
    {
      if (err)
      {
        throw std::runtime_error(err);
      }
    }

    void Load(uint_t soundFreq)
    {
      EmuPtr emu = CreateEmu();
      CheckError(emu->set_sample_rate(soundFreq));
      CheckError(emu->load_mem(Data->Start(), Data->Size()));
      Emu.swap(emu);
      SoundFreq = soundFreq;
    }
  private:
    const EmuCreator CreateEmu;
    const Binary::Data::Ptr Data;
    uint_t SoundFreq;
    EmuPtr Emu;
    uint_t Track;
    mutable Devices::Details::AnalysisMap Analysis;
  };
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(GME::Ptr tune, StateIterator::Ptr iterator, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Tune(tune)
      , Iterator(iterator)
      , State(Iterator->GetStateObserver())
      , SoundParams(Sound::RenderParameters::Create(params))
      , Target(target)
      , Looped()
    {
      ApplyParameters();
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return State;
    }

    virtual Module::Analyzer::Ptr GetAnalyzer() const
    {
      return Tune;
    }

    virtual bool RenderFrame()
    {
      try
      {
        ApplyParameters();

        Sound::ChunkBuilder builder;
        const uint_t samplesPerFrame = SoundParams->SamplesPerFrame();
        builder.Reserve(samplesPerFrame);
        Tune->Render(samplesPerFrame, builder);
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
      SoundParams.Reset();
      Tune->Reset();
      Iterator->Reset();
    }

    virtual void SetPosition(uint_t frame)
    {
      SeekTune(frame);
      Module::SeekIterator(*Iterator, frame);
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Looped = SoundParams->Looped();
      }
    }

    void SeekTune(uint_t frame)
    {
      uint_t current = State->Frame();
      if (frame < current)
      {
        Tune->Reset();
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        Tune->Skip(delta * SoundParams->SamplesPerFrame());
      }
    }
  private:
    const GME::Ptr Tune;
    const StateIterator::Ptr Iterator;
    const TrackState::Ptr State;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    bool Looped;
  };
  
  class Holder : public Module::Holder
  {
  public:
    Holder(GME::Ptr tune, Information::Ptr info, Parameters::Accessor::Ptr props)
      : Tune(tune)
      , Info(info)
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
    const GME::Ptr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
  
  typedef Formats::Chiptune::Decoder::Ptr (*DecoderCreator)();
  typedef Formats::Chiptune::PolyTrackContainer::Ptr (*ParseFunc)(const Binary::Container& data);
  
  struct PluginDescription
  {
    const char* const Id;
    const uint_t Caps;
    const DecoderCreator CreateDecoder;
    const ParseFunc Parse;
    const EmuCreator CreateEmu;
  };
  
  bool HasContainer(const String& type, Parameters::Accessor::Ptr params)
  {
    Parameters::StringType container;
    Require(params->FindValue(Module::ATTR_CONTAINER, container));
    return container == type || boost::algorithm::ends_with(container, ">" + type);
  }
  
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
        if (const Formats::Chiptune::PolyTrackContainer::Ptr container = Desc.Parse(rawData))
        {
          const Parameters::Accessor::Ptr props = propBuilder.GetResult();
          const Sound::RenderParameters::Ptr soundParams = Sound::RenderParameters::Create(props);

          const GME::Ptr tune = boost::make_shared<GME>(Desc.CreateEmu, container, soundParams->SoundFreq());
        
          if (container->TracksCount() > 1)
          {
            Require(HasContainer(Desc.Id, props));
          }
          const uint_t trk = container->StartTrackIndex();
          tune->SetTrack(trk);
        
          const uint_t frames = GetInformation(*tune, propBuilder);
          const Information::Ptr info = CreateStreamInfo(frames);
        
          propBuilder.SetSource(*container);
        
          return boost::make_shared<Holder>(tune, info, props);
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create %1%: %2%", Desc.Id, e.what());
      }
      return Module::Holder::Ptr();
    }
  private:
    static uint_t GetInformation(const GME& gme, PropertiesBuilder& propBuilder)
    {
      static const Time::Milliseconds DEFAULT_DURATION(120000);

      ::track_info_t info;
      gme.GetInfo(info);
      
      const String& system = OptimizeString(FromStdString(info.system));
      const String& song = OptimizeString(FromStdString(info.song));
      const String& game = OptimizeString(FromStdString(info.game));
      const String& author = OptimizeString(FromStdString(info.author));
      const String& comment = OptimizeString(FromStdString(info.comment));
      const String& copyright = OptimizeString(FromStdString(info.copyright));
      const String& dumper = OptimizeString(FromStdString(info.dumper));
      
      if (!system.empty())
      {
        propBuilder.SetValue(ATTR_COMPUTER, system);
      }
      if (!song.empty())
      {
        propBuilder.SetTitle(song);
        propBuilder.SetProgram(game);
      }
      else
      {
        propBuilder.SetTitle(game);
      }
      if (!author.empty())
      {
        propBuilder.SetAuthor(author);
      }
      else
      {
        propBuilder.SetAuthor(dumper);
      }
      if (!comment.empty())
      {
        propBuilder.SetComment(comment);
      }
      else if (!copyright.empty())
      {
        propBuilder.SetComment(copyright);
      }
      
      const Time::Milliseconds duration = info.length > 0 ? Time::Milliseconds(info.length) : DEFAULT_DURATION;
      const Time::Milliseconds period = Time::Milliseconds(20);
      propBuilder.SetValue(Parameters::ZXTune::Sound::FRAMEDURATION, Time::Microseconds(period).Get());
      const uint_t frames = duration.Get() / period.Get();
      return frames;
    }
  private:
    const PluginDescription& Desc;
  };
  
  const PluginDescription PLUGINS[] =
  {
    //nsf
    {
      "NSF",
      ZXTune::Capabilities::Module::Device::RP2A0X,
      &Formats::Chiptune::CreateNSFDecoder,
      &Formats::Chiptune::NSF::Parse,
      &Create< ::Nsf_Emu>,
    }
  };
}
}

namespace ZXTune
{
  void RegisterGMEPlugins(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP;
    for (const Module::GME::PluginDescription* it = Module::GME::PLUGINS; it != boost::end(Module::GME::PLUGINS); ++it)
    {
      const Module::GME::PluginDescription& desc = *it;

      const Formats::Chiptune::Decoder::Ptr decoder = desc.CreateDecoder();
      const Module::Factory::Ptr factory = boost::make_shared<Module::GME::Factory>(desc);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(FromStdString(desc.Id), CAPS | desc.Caps, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
