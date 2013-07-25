/*
Abstract:
  AYM-based players common functionality implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "core/plugins/players/analyzer.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <debug/log.h>
#include <devices/details/parameters_helper.h>
#include <math/numeric.h>
#include <sound/gainer.h>
#include <sound/sound_parameters.h>
#include <sound/mixer_factory.h>
#include <sound/receiver.h>
#include <sound/render_params.h>
//text includes
#include <core/text/core.h>

namespace ZXTune
{
  using namespace Module;

  const Debug::Stream Dbg("Core::AYBase");

  class AYMDataIterator : public AYM::DataIterator
  {
  public:
    AYMDataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr delegate, AYM::DataRenderer::Ptr renderer)
      : Params(trackParams)
      , Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , Render(renderer)
    {
      FillCurrentChunk();
    }

    virtual void Reset()
    {
      Params.Reset();
      Delegate->Reset();
      Render->Reset();
      FillCurrentChunk();
    }

    virtual bool IsValid() const
    {
      return Delegate->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      Delegate->NextFrame(looped);
      FillCurrentChunk();
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return State;
    }

    virtual Devices::AYM::Registers GetData() const
    {
      return CurrentData;
    }
  private:
    void FillCurrentChunk()
    {
      if (Delegate->IsValid())
      {
        SynchronizeParameters();
        AYM::TrackBuilder builder(Table);
        Render->SynthesizeData(*State, builder);
        CurrentData = builder.GetResult();
      }
    }

    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        Params->FreqTable(Table);
      }
    }
  private:
    Devices::Details::ParametersHelper<AYM::TrackParameters> Params;
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const AYM::DataRenderer::Ptr Render;
    Devices::AYM::Registers CurrentData;
    FrequencyTable Table;
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
        Devices::AYM::DataChunk chunk;
        chunk.TimeStamp = FlushChunk.TimeStamp;
        chunk.Data = Iterator->GetData();
        CommitChunk(chunk);
        Iterator->NextFrame(Looped);
        return Iterator->IsValid();
      }
      return false;
    }

    virtual void Reset()
    {
      Params.Reset();
      Iterator->Reset();
      Device->Reset();
      FlushChunk = Devices::AYM::DataChunk();
      FrameDuration = Devices::AYM::Stamp();
      Looped = false;
    }

    virtual void SetPosition(uint_t frameNum)
    {
      SeekIterator(*Iterator, frameNum);
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

    void CommitChunk(const Devices::AYM::DataChunk& chunk)
    {
      Device->RenderData(chunk);
      FlushChunk.TimeStamp += FrameDuration;
      Device->RenderData(FlushChunk);
      Device->Flush();
    }
  private:
    Devices::Details::ParametersHelper<Sound::RenderParameters> Params;
    const AYM::DataIterator::Ptr Iterator;
    const Devices::AYM::Device::Ptr Device;
    Devices::AYM::DataChunk FlushChunk;
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
      const uint_t frameDuration = Sound::RenderParameters::Create(params)->FrameDuration().Get();
      const uint_t fadeInFrames = (fadeIn + frameDuration) / frameDuration;
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
    const uint_t frameDuration = Sound::RenderParameters::Create(params)->FrameDuration().Get();
    const uint_t fadeOutFrames = (fadeOut + frameDuration) / frameDuration;
    const uint_t totalFrames = info->FramesCount();
    const uint_t startFadingFrame = totalFrames - fadeOutFrames;
    Dbg("Fade out for %1% frames starting from %2% frame out of %3%", fadeOutFrames, startFadingFrame, totalFrames);
    return boost::make_shared<FadeoutFilter>(startFadingFrame, fadeOutFrames, target);
  }

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
      const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
      const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
      const AYM::DataIterator::Ptr iterator = Tune->CreateDataIterator(trackParams);
      return AYM::CreateRenderer(renderParams, iterator, chip);
    }

    virtual AYM::Chiptune::Ptr GetChiptune() const
    {
      return Tune;
    }
  private:
    const AYM::Chiptune::Ptr Tune;
  };

  Devices::AYM::Chip::Ptr CreateChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    typedef Sound::ThreeChannelsMatrixMixer MixerType;
    const MixerType::Ptr mixer = MixerType::Create();
    const Parameters::Accessor::Ptr pollParams = Sound::CreateMixerNotificationParameters(params, mixer);
    const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(pollParams);
    return Devices::AYM::CreateChip(chipParams, mixer, target);
  }

  class StreamDataIterator : public AYM::DataIterator
  {
  public:
    StreamDataIterator(StateIterator::Ptr delegate, AYM::RegistersArrayPtr data)
      : Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , Data(data)
    {
      UpdateCurrentState();
    }

    virtual void Reset()
    {
      CurrentState = Devices::AYM::Registers();
      Delegate->Reset();
      UpdateCurrentState();
    }

    virtual bool IsValid() const
    {
      return Delegate->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      Delegate->NextFrame(looped);
      UpdateCurrentState();
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return State;
    }

    virtual Devices::AYM::Registers GetData() const
    {
      return CurrentState;
    }
  private:
    void UpdateCurrentState()
    {
      if (Delegate->IsValid())
      {
        const Devices::AYM::Registers& inRegs = (*Data)[State->Frame()];
        CurrentState.Reset(Devices::AYM::Registers::ENV);
        for (Devices::AYM::Registers::IndicesIterator it(inRegs); it; ++it)
        {
          CurrentState[*it] = inRegs[*it];
        }
      }
    }
  private:
    const StateIterator::Ptr Delegate;
    const TrackState::Ptr State;
    const AYM::RegistersArrayPtr Data;
    Devices::AYM::Registers CurrentState;
  };

  class AYMStreamedChiptune : public AYM::Chiptune
  {
  public:
    AYMStreamedChiptune(AYM::RegistersArrayPtr data, Parameters::Accessor::Ptr properties, uint_t loopFrame)
      : Data(data)
      , Properties(properties)
      , Info(CreateStreamInfo(Data->size(), loopFrame))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr /*trackParams*/) const
    {
      const StateIterator::Ptr iter = CreateStreamStateIterator(Info);
      return boost::make_shared<StreamDataIterator>(iter, Data);
    }
  private:
    const AYM::RegistersArrayPtr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace ZXTune
{
  namespace Module
  {
    namespace AYM
    {
      void ChannelBuilder::SetTone(int_t halfTones, int_t offset)
      {
        const int_t halftone = Math::Clamp<int_t>(halfTones, 0, static_cast<int_t>(Table.size()) - 1);
        const uint_t tone = (Table[halftone] + offset) & 0xfff;
        SetTone(tone);
      }

      void ChannelBuilder::SetTone(uint_t tone)
      {
        using namespace Devices::AYM;
        const uint_t reg = Registers::TONEA_L + 2 * Channel;
        Data[static_cast<Registers::Index>(reg)] = static_cast<uint8_t>(tone & 0xff);
        Data[static_cast<Registers::Index>(reg + 1)] = static_cast<uint8_t>(tone >> 8);
      }

      void ChannelBuilder::SetLevel(int_t level)
      {
        using namespace Devices::AYM;
        const uint_t reg = Registers::VOLA + Channel;
        Data[static_cast<Registers::Index>(reg)] = static_cast<uint8_t>(Math::Clamp<int_t>(level, 0, 15));
      }

      void ChannelBuilder::DisableTone()
      {
        Data[Devices::AYM::Registers::MIXER] |= (Devices::AYM::Registers::MASK_TONEA << Channel);
      }

      void ChannelBuilder::EnableEnvelope()
      {
        using namespace Devices::AYM;
        const uint_t reg = Registers::VOLA + Channel;
        Data[static_cast<Registers::Index>(reg)] |= Registers::MASK_ENV;
      }

      void ChannelBuilder::DisableNoise()
      {
        Data[Devices::AYM::Registers::MIXER] |= (Devices::AYM::Registers::MASK_NOISEA << Channel);
      }

      void TrackBuilder::SetNoise(uint_t level)
      {
        Data[Devices::AYM::Registers::TONEN] = static_cast<uint8_t>(level & 31);
      }

      void TrackBuilder::SetEnvelopeType(uint_t type)
      {
        Data[Devices::AYM::Registers::ENV] = static_cast<uint8_t>(type);
      }

      void TrackBuilder::SetEnvelopeTone(uint_t tone)
      {
        Data[Devices::AYM::Registers::TONEE_L] = static_cast<uint8_t>(tone & 0xff);
        Data[Devices::AYM::Registers::TONEE_H] = static_cast<uint8_t>(tone >> 8);
      }

      uint_t TrackBuilder::GetFrequency(int_t halfTone) const
      {
        return Table[halfTone];
      }

      int_t TrackBuilder::GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const
      {
        const int_t halfFrom = Math::Clamp<int_t>(halfToneFrom, 0, static_cast<int_t>(Table.size()) - 1);
        const int_t halfTo = Math::Clamp<int_t>(halfToneTo, 0, static_cast<int_t>(Table.size()) - 1);
        const int_t toneFrom = Table[halfFrom];
        const int_t toneTo = Table[halfTo];
        return toneTo - toneFrom;
      }

      Analyzer::Ptr CreateAnalyzer(Devices::AYM::Device::Ptr device)
      {
        if (Devices::StateSource::Ptr src = boost::dynamic_pointer_cast<Devices::StateSource>(device))
        {
          return Module::CreateAnalyzer(src);
        }
        return Analyzer::Ptr();
      }

      DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
      {
        return boost::make_shared<AYMDataIterator>(trackParams, iterator, renderer);
      }

      Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device)
      {
        return boost::make_shared<AYMRenderer>(params, iterator, device);
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

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
      {
        return boost::make_shared<AYMHolder>(chiptune);
      }

      Chiptune::Ptr CreateStreamedChiptune(RegistersArrayPtr data, Parameters::Accessor::Ptr properties, uint_t loopFrame)
      {
        return boost::make_shared<AYMStreamedChiptune>(data, properties, loopFrame);
      }
    }
  }
}
