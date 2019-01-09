/**
* 
* @file
*
* @brief  AHX chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "abysshighestexperience.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <debug/log.h>
#include <formats/chiptune/digital/abysshighestexperience.h>
#include <module/players/analyzer.h>
#include <module/players/properties_meta.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//3rdparty
#include <3rdparty/hvl/hvl_replay.h>
//text includes
#include <module/text/platforms.h>

namespace Module
{
namespace AHX
{
  const Debug::Stream Dbg("Core::AHXSupp");

  typedef std::shared_ptr<hvl_tune> HvlPtr;
  
  enum StereoSeparation
  {
    MONO = 0,
    STEREO = 4
  };
  
  HvlPtr LoadModule(const Binary::Data& data)
  {
    static bool initialized = false;
    if (!initialized)
    {
      hvl_InitReplayer();
      initialized = true;
    }
    const HvlPtr result = HvlPtr(hvl_ParseTune(static_cast<const uint8*>(data.Start()), data.Size(), Parameters::ZXTune::Sound::FREQUENCY_DEFAULT, MONO), &hvl_FreeTune);
    Require(result.get() != nullptr);
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

    uint_t PositionsCount() const override
    {
      return Hvl->ht_PositionNr;
    }

    uint_t LoopPosition() const override
    {
      return Hvl->ht_PosJump;
    }

    uint_t FramesCount() const override
    {
      if (!CachedFramesCount)
      {
        FillCache();
      }
      return CachedFramesCount;
    }

    uint_t LoopFrame() const override
    {
      if (!CachedLoopFrame)
      {
        FillCache();
      }
      return CachedLoopFrame;
    }
    
    uint_t ChannelsCount() const override
    {
      return Hvl->ht_Channels;
    }

    uint_t Tempo() const override
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
      : Hvl(std::move(hvl))
    {
    }

    uint_t Position() const override
    {
      return Hvl->ht_PosNr;
    }

    uint_t Pattern() const override
    {
      return Hvl->ht_PosNr;//TODO
    }

    uint_t Line() const override
    {
      return Hvl->ht_NoteNr;
    }

    uint_t Tempo() const override
    {
      return Hvl->ht_Tempo;
    }

    uint_t Quirk() const override
    {
      return Hvl->ht_Tempo - Hvl->ht_StepWaitFrames;
    }

    uint_t Frame() const override
    {
      return Hvl->ht_PlayingTime / Hvl->ht_SpeedMultiplier;
    }

    uint_t Channels() const override
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
      : Hvl(std::move(hvl))
    {
    }

    std::vector<ChannelState> GetState() const override
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
      return result;
    }
  private:
    const HvlPtr Hvl;
  };

  class HVL
  {
  public:
    typedef std::shared_ptr<HVL> Ptr;
    
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
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
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
      : Tune(std::move(tune))
      , Target(std::move(target))
      , SoundParams(Sound::RenderParameters::Create(std::move(params)))
      , Looped()
    {
      ApplyParameters();
    }

    TrackState::Ptr GetTrackState() const override
    {
      return Tune->MakeTrackState();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return Tune->MakeAnalyzer();
    }

    bool RenderFrame() override
    {
      try
      {
        ApplyParameters();

        Sound::ChunkBuilder builder;
        Tune->RenderFrame(builder);
        Target->ApplyData(builder.CaptureResult());
        return Looped || !Tune->EndReached();
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      SoundParams.Reset();
      Tune->Reset();
    }

    void SetPosition(uint_t frame) override
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
      : Tune(std::move(tune))
      , Info(std::move(info))
      , Properties(std::move(props))
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
      return MakePtr<Renderer>(Tune, std::move(target), std::move(params));
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

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }
  private:
    MetaProperties Meta;
  };
  
  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr decoder)
      : Decoder(std::move(decoder))
    {
    }
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Decoder->Parse(rawData, dataBuilder))
        {
          props.SetSource(*container);
          props.SetPlatform(Platforms::AMIGA);

          auto tune = MakePtr<HVL>(*container);
          auto info = MakePtr<Information>(*container);
          return MakePtr<Holder>(std::move(tune), std::move(info), properties);
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create AHX: %s", e.what());
      }
      return Module::Holder::Ptr();
    }
    
  private:
    const Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr Decoder;
  };
  
  Factory::Ptr CreateFactory(Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(std::move(decoder));
  }
}
}
