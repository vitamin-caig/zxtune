/**
* 
* @file
*
* @brief  AHX support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/analyzer.h"
#include "core/plugins/players/properties_meta.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/digital/abysshighestexperience.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//3rdparty
#include <3rdparty/hvl/replay.h>

namespace
{
  const Debug::Stream Dbg("Core::AHXSupp");
}

namespace Module
{
namespace AHX
{
  typedef boost::shared_ptr<hvl_tune> HvlPtr;
  
  HvlPtr LoadModule(const Binary::Data& data)
  {
    static bool initialized = false;
    if (!initialized)
    {
      hvl_InitReplayer();
      initialized = true;
    }
    const HvlPtr result = HvlPtr(hvl_LoadTune(static_cast<const uint8*>(data.Start()), data.Size(), 4, Parameters::ZXTune::Sound::FREQUENCY_DEFAULT), &hvl_FreeTune);
    Require(result.get() != 0);
    return result;
  }
  
  class Information : public Module::Information
  {
  public:
    explicit Information(const Binary::Data& data)
      : Hvl(LoadModule(data))
      , CachedFramesCount()
      , CachedLoopFrame()
    {
    }

    virtual uint_t PositionsCount() const
    {
      return Hvl->ht_PositionNr;
    }

    virtual uint_t LoopPosition() const
    {
      return Hvl->ht_PosJump;
    }

    virtual uint_t PatternsCount() const
    {
      return Hvl->ht_TrackNr;//????
    }

    virtual uint_t FramesCount() const
    {
      if (!CachedFramesCount)
      {
        FillCache();
      }
      return CachedFramesCount;
    }

    virtual uint_t LoopFrame() const
    {
      if (!CachedLoopFrame)
      {
        FillCache();
      }
      return CachedLoopFrame;
    }
    
    virtual uint_t ChannelsCount() const
    {
      return Hvl->ht_Channels;
    }

    virtual uint_t Tempo() const
    {
      return Hvl->ht_Tempo;
    }
  private:
    void FillCache() const
    {
      while (!Hvl->ht_SongEndReached)
      {
        hvl_NextFrame(Hvl.get());
        ++CachedFramesCount;
        if (Hvl->ht_PosNr == Hvl->ht_Restart && 0 == Hvl->ht_NoteNr && Hvl->ht_Tempo == Hvl->ht_StepWaitFrames)
        {
          CachedLoopFrame = CachedFramesCount;
        }
      }
    }
  private:
    const HvlPtr Hvl;
    mutable uint_t CachedFramesCount;
    mutable uint_t CachedLoopFrame;
  };
  
  class TrackState : public Module::TrackState
  {
  public:
    explicit TrackState(HvlPtr hvl)
      : Hvl(hvl)
    {
    }

    virtual uint_t Position() const
    {
      return Hvl->ht_PosNr;
    }

    virtual uint_t Pattern() const
    {
      return Hvl->ht_PosNr;//TODO
    }

    virtual uint_t PatternSize() const
    {
      return Hvl->ht_TrackLength;
    }

    virtual uint_t Line() const
    {
      return Hvl->ht_NoteNr;
    }

    virtual uint_t Tempo() const
    {
      return Hvl->ht_Tempo;
    }

    virtual uint_t Quirk() const
    {
      return Hvl->ht_Tempo - Hvl->ht_StepWaitFrames;
    }

    virtual uint_t Frame() const
    {
      return Hvl->ht_PlayingTime / Hvl->ht_SpeedMultiplier;
    }

    virtual uint_t Channels() const
    {
      uint_t result = 0;
      for (uint_t idx = 0, lim = Hvl->ht_Channels; idx != lim; ++idx)
      {
        result += Hvl->ht_Voices[idx].vc_TrackOn != 0;
      }
      return result;
    }
  private:
    const HvlPtr Hvl;
  };
  
  class Analyzer : public Module::Analyzer
  {
  public:
    explicit Analyzer(HvlPtr hvl)
      : Hvl(hvl)
    {
    }

    virtual void GetState(std::vector<ChannelState>& channels) const
    {
      std::vector<ChannelState> result;
      result.reserve(Hvl->ht_Channels);
      for (uint_t idx = 0, lim = Hvl->ht_Channels; idx != lim; ++idx)
      {
        const hvl_voice& voice = Hvl->ht_Voices[idx];
        if (const int_t volume = voice.vc_VoiceVolume)
        {
          ChannelState state;
          state.Band = voice.vc_TrackPeriod;
          state.Level = volume >= 64 ? 100 : volume * 100 / 64;
          result.push_back(state);
        }
      }
      channels.swap(result);
    }
  private:
    const HvlPtr Hvl;
  };

  class HVL
  {
  public:
    typedef boost::shared_ptr<HVL> Ptr;
    
    explicit HVL(const Binary::Data& data)
      : Hvl(LoadModule(data))
      , SamplesPerFrame()
    {
      Reset();
    }
    
    void Reset()
    {
      hvl_InitSubsong(Hvl.get(), 0);
      SamplesPerFrame = Hvl->ht_Frequency / 50;
    }
    
    void SetFrequency(uint_t freq)
    {
      if (freq != Hvl->ht_Frequency)
      {
        Require(Hvl->ht_PlayingTime == 0);
        Hvl->ht_Frequency = freq;
        Reset();
      }
    }
    
    void RenderFrame(Sound::ChunkBuilder& target)
    {
      BOOST_STATIC_ASSERT(Sound::Sample::CHANNELS == 2);
      BOOST_STATIC_ASSERT(Sound::Sample::BITS == 16);
      BOOST_STATIC_ASSERT(Sound::Sample::MID == 0);
      target.Reserve(SamplesPerFrame);
      void* const buf = target.Allocate(SamplesPerFrame);
      hvl_DecodeFrame(Hvl.get(), static_cast<int8*>(buf), static_cast<int8*>(buf) + sizeof(Sound::Sample) / 2, sizeof(Sound::Sample));
    }
    
    void Seek(uint_t frame)
    {
      uint_t current = Hvl->ht_PlayingTime;
      if (frame < current)
      {
        Reset();
        current = 0;
      }
      for (; current < frame; ++current)
      {
        hvl_NextFrame(Hvl.get());
      }
    }
    
    bool EndReached() const
    {
      return 0 != Hvl->ht_SongEndReached;
    }
    
    TrackState::Ptr MakeTrackState() const
    {
      return MakePtr<TrackState>(Hvl);
    }

    Analyzer::Ptr MakeAnalyzer() const
    {
      return MakePtr<Analyzer>(Hvl);
    } 
  private:
    const HvlPtr Hvl;
    uint_t SamplesPerFrame;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(HVL::Ptr tune, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Tune(tune)
      , Target(target)
      , SoundParams(Sound::RenderParameters::Create(params))
      , Looped()
    {
      ApplyParameters();
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return Tune->MakeTrackState();
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return Tune->MakeAnalyzer();
    }

    virtual bool RenderFrame()
    {
      try
      {
        ApplyParameters();

        Sound::ChunkBuilder builder;
        Tune->RenderFrame(builder);
        Target->ApplyData(builder.GetResult());
        return Looped || !Tune->EndReached();
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
    }

    virtual void SetPosition(uint_t frame)
    {
      Tune->Seek(frame);
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Looped = SoundParams->Looped();
        Tune->SetFrequency(SoundParams->SoundFreq());
      }
    }
  private:
    const HVL::Ptr Tune;
    const Sound::Receiver::Ptr Target;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    bool Looped;
  };
  
  class Holder : public Module::Holder
  {
  public:
    Holder(HVL::Ptr tune, Module::Information::Ptr info, Parameters::Accessor::Ptr props)
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
      return MakePtr<Renderer>(Tune, target, params);
    }
  private:
    const HVL::Ptr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
  
  class DataBuilder : public Formats::Chiptune::AbyssHighestExperience::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Meta(props)
    {
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return Meta;
    }
  private:
    MetaProperties Meta;
  };
  
  class Factory : public Module::Factory
  {
  public:
    virtual Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData, Parameters::Container::Ptr properties) const
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::AbyssHighestExperience::Parse(rawData, dataBuilder))
        {
          props.SetSource(*container);

          const HVL::Ptr tune = MakePtr<HVL>(*container);
          const Information::Ptr info = MakePtr<Information>(*container);
          return MakePtr<Holder>(tune, info, properties);
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create AHX: %s", e.what());
      }
      return Module::Holder::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterAHXSupport(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'A', 'H', 'X', 0};
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateAbyssHighestExperienceDecoder();
    const Module::AHX::Factory::Ptr factory = MakePtr<Module::AHX::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
