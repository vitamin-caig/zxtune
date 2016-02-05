/**
* 
* @file
*
* @brief  AYM-based chiptunes common functionality implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_base.h"
#include "core/plugins/players/analyzer.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <math/numeric.h>
#include <parameters/tracking_helper.h>
#include <sound/gainer.h>
#include <sound/mixer_factory.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/weak_ptr.hpp>

namespace
{
  const Debug::Stream Dbg("Core::AYBase");
}

namespace Module
{
  class AYMHolder : public AYM::Holder
  {
  public:
    explicit AYMHolder(AYM::Chiptune::Ptr chiptune)
      : Tune(chiptune)
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Tune->GetInformation();
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Tune->GetProperties();
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      return AYM::CreateRenderer(*this, params, target);
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const
    {
      const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
      const AYM::DataIterator::Ptr iterator = Tune->CreateDataIterator(trackParams);
      const Sound::RenderParameters::Ptr soundParams = Sound::RenderParameters::Create(params);
      return AYM::CreateRenderer(soundParams, iterator, chip);
    }

    virtual AYM::Chiptune::Ptr GetChiptune() const
    {
      return Tune;
    }
  private:
    const AYM::Chiptune::Ptr Tune;
  };

  class AYMRenderer : public Renderer
  {
  public:
    AYMRenderer(Sound::RenderParameters::Ptr params, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device)
      : Params(params)
      , Iterator(iterator)
      , Device(device)
      , FrameDuration()
      , Looped()
    {
#ifndef NDEBUG
//perform self-test
      for (; Iterator->IsValid(); Iterator->NextFrame(false));
      Iterator->Reset();
#endif
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return Iterator->GetStateObserver();
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return AYM::CreateAnalyzer(Device);
    }

    virtual bool RenderFrame()
    {
      if (Iterator->IsValid())
      {
        SynchronizeParameters();
        if (LastChunk.TimeStamp == Devices::AYM::Stamp())
        {
          //first chunk
          TransferChunk();
        }
        Iterator->NextFrame(Looped);
        LastChunk.TimeStamp += FrameDuration;
        TransferChunk();
      }
      return Iterator->IsValid();
    }

    virtual void Reset()
    {
      Params.Reset();
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = Devices::AYM::Stamp();
      FrameDuration = Devices::AYM::Stamp();
      Looped = false;
    }

    virtual void SetPosition(uint_t frameNum)
    {
      const TrackState::Ptr state = Iterator->GetStateObserver();
      uint_t curFrame = state->Frame();
      if (curFrame > frameNum)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = Devices::AYM::Stamp();
        curFrame = 0;
      }
      while (curFrame < frameNum && Iterator->IsValid())
      {
        TransferChunk();
        Iterator->NextFrame(true);
        ++curFrame;
      }
    }
  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        FrameDuration = Params->FrameDuration();
        Looped = Params->Looped();
      }
    }

    void TransferChunk()
    {
      LastChunk.Data = Iterator->GetData();
      Device->RenderData(LastChunk);
    }
  private:
    Parameters::TrackingHelper<Sound::RenderParameters> Params;
    const AYM::DataIterator::Ptr Iterator;
    const Devices::AYM::Device::Ptr Device;
    Devices::AYM::DataChunk LastChunk;
    Devices::AYM::Stamp FrameDuration;
    bool Looped;
  };

  class FadeoutFilter : public Sound::Receiver
  {
  public:
    typedef boost::shared_ptr<FadeoutFilter> Ptr;

    FadeoutFilter(uint_t start, uint_t duration, Sound::FadeGainer::Ptr target)
      : Start(start)
      , Duration(duration)
      , Target(target)
    {
    }

    virtual void ApplyData(const Sound::Chunk::Ptr& chunk)
    {
      Target->ApplyData(chunk);
    }

    virtual void Flush()
    {
      if (const TrackState::Ptr state = State.lock())
      {
        if (state->Frame() >= Start)
        {
          Target->SetFading(Sound::Gain::Type(-1), Duration);
          State = boost::weak_ptr<TrackState>();
        }
      }
      Target->Flush();
    }

    void SetTrackState(TrackState::Ptr state)
    {
      State = state;
    }
  private:
    const uint_t Start;
    const uint_t Duration;
    const Sound::FadeGainer::Ptr Target;
    boost::weak_ptr<const TrackState> State;
  };

  Sound::FadeGainer::Ptr CreateFadeGainer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    Parameters::IntType fadeIn = Parameters::ZXTune::Sound::FADEIN_DEFAULT;
    Parameters::IntType fadeOut = Parameters::ZXTune::Sound::FADEOUT_DEFAULT;
    params->FindValue(Parameters::ZXTune::Sound::FADEIN, fadeIn);
    params->FindValue(Parameters::ZXTune::Sound::FADEOUT, fadeOut);
    if (fadeIn == 0 && fadeOut == 0)
    {
      return Sound::FadeGainer::Ptr();
    }
    Dbg("Using fade in for %1% uS, fade out for %2% uS", fadeIn, fadeOut);
    const Sound::FadeGainer::Ptr gainer = Sound::CreateFadeGainer();
    gainer->SetTarget(target);
    if (fadeIn != 0)
    {
      gainer->SetGain(Sound::Gain::Type());
      const Parameters::IntType frameDuration = Sound::RenderParameters::Create(params)->FrameDuration().Get();
      const uint_t fadeInFrames = static_cast<uint_t>((fadeIn + frameDuration) / frameDuration);
      Dbg("Fade in for %1% frames", fadeInFrames);
      gainer->SetFading(Sound::Gain::Type(1), fadeInFrames);
    }
    else
    {
      gainer->SetGain(Sound::Gain::Type(1));
    }
    return gainer;
  }

  FadeoutFilter::Ptr CreateFadeOutFilter(Parameters::Accessor::Ptr params, Information::Ptr info, Sound::FadeGainer::Ptr target)
  {
    Parameters::IntType fadeOut = Parameters::ZXTune::Sound::FADEOUT_DEFAULT;
    params->FindValue(Parameters::ZXTune::Sound::FADEOUT, fadeOut);
    if (fadeOut == 0)
    {
      return FadeoutFilter::Ptr();
    }
    const Parameters::IntType frameDuration = Sound::RenderParameters::Create(params)->FrameDuration().Get();
    const uint_t fadeOutFrames = static_cast<uint_t>((fadeOut + frameDuration) / frameDuration);
    const uint_t totalFrames = info->FramesCount();
    const uint_t startFadingFrame = totalFrames - fadeOutFrames;
    Dbg("Fade out for %1% frames starting from %2% frame out of %3%", fadeOutFrames, startFadingFrame, totalFrames);
    return MakePtr<FadeoutFilter>(startFadingFrame, fadeOutFrames, target);
  }

  class StubAnalyzer : public Module::Analyzer
  {
  public:
    virtual void GetState(std::vector<Module::Analyzer::ChannelState>& channels) const
    {
      channels.clear();
    }
  };
}

namespace Module
{
  namespace AYM
  {
    Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
    {
      return MakePtr<AYMHolder>(chiptune);
    }

    Analyzer::Ptr CreateAnalyzer(Devices::AYM::Device::Ptr device)
    {
      if (Devices::StateSource::Ptr src = boost::dynamic_pointer_cast<Devices::StateSource>(device))
      {
        return Module::CreateAnalyzer(src);
      }
      else
      {
        return MakePtr<StubAnalyzer>();
      }
    }

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device)
    {
      return MakePtr<AYMRenderer>(params, iterator, device);
    }
    
    Devices::AYM::Chip::Ptr CreateChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
    {
      typedef Sound::ThreeChannelsMatrixMixer MixerType;
      const MixerType::Ptr mixer = MixerType::Create();
      const Parameters::Accessor::Ptr pollParams = Sound::CreateMixerNotificationParameters(params, mixer);
      const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(pollParams);
      return Devices::AYM::CreateChip(chipParams, mixer, target);
    }

    Renderer::Ptr CreateRenderer(const Holder& holder, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
    {
      if (const Sound::FadeGainer::Ptr fade = CreateFadeGainer(params, target))
      {
        if (FadeoutFilter::Ptr fadeout = CreateFadeOutFilter(params, holder.GetModuleInformation(), fade))
        {
          const Devices::AYM::Chip::Ptr chip = CreateChip(params, fadeout);
          const Renderer::Ptr result = holder.CreateRenderer(params, chip);
          fadeout->SetTrackState(result->GetTrackState());
          return result;
        }
        else
        {
          //only fade in
          const Devices::AYM::Chip::Ptr chip = CreateChip(params, fade);
          return holder.CreateRenderer(params, chip);
        }
      }
      else
      {
        const Devices::AYM::Chip::Ptr chip = CreateChip(params, target);
        return holder.CreateRenderer(params, chip);
      }
    }
  }
}
