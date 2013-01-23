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
#include "ay_conversion.h"
//common includes
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <sound/gainer.h>
#include <sound/mixer_factory.h>
#include <sound/sound_parameters.h>
#include <sound/receiver.h>
#include <sound/render_params.h>
#include <sound/sample_convert.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG 7D6A549C

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Debug::Stream Dbg("Core::AYBase");

  class AYMDataIterator : public AYM::DataIterator
  {
  public:
    AYMDataIterator(AYM::TrackParameters::Ptr trackParams, StateIterator::Ptr delegate, AYM::DataRenderer::Ptr renderer)
      : TrackParams(trackParams)
      , Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , Render(renderer)
    {
      FillCurrentChunk();
    }

    virtual void Reset()
    {
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

    virtual void GetData(Devices::AYM::DataChunk& chunk) const
    {
      chunk = CurrentChunk;
    }
  private:
    void FillCurrentChunk()
    {
      if (Delegate->IsValid())
      {
        AYM::TrackBuilder builder(TrackParams->FreqTable());
        Render->SynthesizeData(*State, builder);
        builder.GetResult(CurrentChunk);
      }
    }
  private:
    const AYM::TrackParameters::Ptr TrackParams;
    const StateIterator::Ptr Delegate;
    const TrackState::Ptr State;
    const AYM::DataRenderer::Ptr Render;
    Devices::AYM::DataChunk CurrentChunk;
  };

  class AYMReceiver : public Devices::AYM::Receiver
  {
  public:
    explicit AYMReceiver(Sound::MultichannelReceiver::Ptr target)
      : Target(target)
      , Data(Devices::AYM::CHANNELS)
    {
    }

    virtual void ApplyData(const Devices::AYM::MultiSample& data)
    {
      std::transform(data.begin(), data.end(), Data.begin(), &Sound::ToSample<Devices::AYM::Sample>);
      Target->ApplyData(Data);
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const Sound::MultichannelReceiver::Ptr Target;
    Sound::MultichannelSample Data;
  };

  class AYMAnalyzer : public Analyzer
  {
  public:
    explicit AYMAnalyzer(Devices::AYM::Chip::Ptr device)
      : Device(device)
    {
    }

    //analyzer virtuals
    virtual uint_t ActiveChannels() const
    {
      Devices::AYM::ChannelsState state;
      Device->GetState(state);
      return static_cast<uint_t>(std::count_if(state.begin(), state.end(),
        boost::mem_fn(&Devices::AYM::ChanState::Enabled)));
    }

    virtual void BandLevels(std::vector<std::pair<uint_t, uint_t> >& bandLevels) const
    {
      Devices::AYM::ChannelsState state;
      Device->GetState(state);
      bandLevels.resize(state.size());
      std::transform(state.begin(), state.end(), bandLevels.begin(),
        boost::bind(&std::make_pair<uint_t, uint_t>, boost::bind(&Devices::AYM::ChanState::Band, _1), boost::bind(&Devices::AYM::ChanState::LevelInPercents, _1)));
    }
  private:
    const Devices::AYM::Chip::Ptr Device;
  };

  class AYMRenderer : public Renderer
  {
  public:
    AYMRenderer(AYM::TrackParameters::Ptr params, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device)
      : Params(params)
      , Iterator(iterator)
      , Device(device)
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
        Devices::AYM::DataChunk chunk;
        Iterator->GetData(chunk);
        chunk.TimeStamp = FlushChunk.TimeStamp;
        CommitChunk(chunk);
        Iterator->NextFrame(Params->Looped());
        return Iterator->IsValid();
      }
      return false;
    }

    virtual void Reset()
    {
      Iterator->Reset();
      Device->Reset();
      FlushChunk = Devices::AYM::DataChunk();
    }

    virtual void SetPosition(uint_t frameNum)
    {
      SeekIterator(*Iterator, frameNum);
    }
  private:
    void CommitChunk(const Devices::AYM::DataChunk& chunk)
    {
      Device->RenderData(chunk);
      FlushChunk.TimeStamp += Params->FrameDuration();
      Device->RenderData(FlushChunk);
      Device->Flush();
    }
  private:
    const AYM::TrackParameters::Ptr Params;
    const AYM::DataIterator::Ptr Iterator;
    const Devices::AYM::Device::Ptr Device;
    Devices::AYM::DataChunk FlushChunk;
  };

  class FadeoutFilter : public Sound::Receiver
  {
  public:
    FadeoutFilter(TrackState::Ptr state, uint_t startFrame, uint_t samplesToFade, Sound::FadeGainer::Ptr target)
      : State(state)
      , Start(startFrame)
      , Fading(samplesToFade)
      , Target(target)
    {
    }

    virtual void ApplyData(const Sound::MultiSample& sample)
    {
      Target->ApplyData(sample);
    }

    virtual void Flush()
    {
      if (Start == State->Frame())
      {
        Target->SetFading(Sound::Gain(-1), Fading);
      }
      Target->Flush();
    }
  private:
    const TrackState::Ptr State;
    uint_t Start;
    const uint_t Fading;
    const Sound::FadeGainer::Ptr Target;
  };

  Sound::Receiver::Ptr CreateFadingTarget(Parameters::Accessor::Ptr params, Information::Ptr info, TrackState::Ptr state, Sound::Receiver::Ptr target)
  {
    Parameters::IntType fadeIn = Parameters::ZXTune::Sound::FADEIN_DEFAULT;
    Parameters::IntType fadeOut = Parameters::ZXTune::Sound::FADEOUT_DEFAULT;
    params->FindValue(Parameters::ZXTune::Sound::FADEIN, fadeIn);
    params->FindValue(Parameters::ZXTune::Sound::FADEOUT, fadeOut);
    if (fadeIn == 0 && fadeOut == 0)
    {
      return target;
    }
    Dbg("Using fade in for %1% uS, fade out for %2% uS", fadeIn, fadeOut);
    const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
    const Sound::FadeGainer::Ptr gainer = Sound::CreateFadeGainer();
    gainer->SetTarget(target);
    const Time::Microseconds frameDuration = renderParams->FrameDuration();
    if (fadeIn != 0)
    {
      gainer->SetGain(Sound::Gain());
      const uint_t fadeInSamples = fadeIn * renderParams->SoundFreq() / frameDuration.PER_SECOND;
      Dbg("Fade in for %1% samples", fadeInSamples);
      gainer->SetFading(Sound::Gain(1), fadeInSamples);
    }
    else
    {
      gainer->SetGain(Sound::Gain(1));
    }
    if (fadeOut != 0)
    {
      const uint_t fadeOutFrames = 1 + fadeOut / frameDuration.Get();
      const uint_t fadeOutSamples = fadeOut * renderParams->SoundFreq() / frameDuration.PER_SECOND;
      const uint_t totalFrames = info->FramesCount();
      const uint_t startFadingFrame = totalFrames - fadeOutFrames;
      Dbg("Fade out for %1% samples starting from %2% frame out of %3%", fadeOutSamples, startFadingFrame, totalFrames);
      return boost::make_shared<FadeoutFilter>(state, startFadingFrame, fadeOutSamples, gainer);
    }
    else
    {
      return gainer;
    }
  }

  class AYMHolder : public Holder
  {
  public:
    explicit AYMHolder(AYM::Chiptune::Ptr chiptune)
      : Tune(chiptune)
    {
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Tune->GetProperties()->GetPlugin();
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
      const Sound::Mixer::Ptr mixer = Sound::CreatePollingMixer(Devices::AYM::CHANNELS, Tune->GetProperties());
      const Devices::AYM::Receiver::Ptr receiver = AYM::CreateReceiver(mixer);
      const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(params);
      const Devices::AYM::Chip::Ptr chip = Devices::AYM::CreateChip(chipParams, receiver);
      const Renderer::Ptr result = Tune->CreateRenderer(params, chip);
      const Sound::Receiver::Ptr fading = CreateFadingTarget(params, Tune->GetInformation(), result->GetTrackState(), target);
      mixer->SetTarget(fading);
      return result;
    }

    virtual Binary::Data::Ptr Convert(const Conversion::Parameter& spec, Parameters::Accessor::Ptr params) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&spec))
      {
        return Tune->GetProperties()->GetData();
      }
      else if (const Binary::Data::Ptr res = ConvertAYMFormat(*Tune, spec, Parameters::CreateMergedAccessor(params, Tune->GetProperties())))
      {
        return res;
      }
      throw CreateUnsupportedConversionError(THIS_LINE, spec);
    }
  private:
    const AYM::Chiptune::Ptr Tune;
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
        const uint_t reg = Devices::AYM::DataChunk::REG_TONEA_L + 2 * Channel;
        Chunk.Data[reg] = static_cast<uint8_t>(tone & 0xff);
        Chunk.Data[reg + 1] = static_cast<uint8_t>(tone >> 8);
        Chunk.Mask |= (1 << reg) | (1 << (reg + 1));
      }

      void ChannelBuilder::SetLevel(int_t level)
      {
        const uint_t reg = Devices::AYM::DataChunk::REG_VOLA + Channel;
        Chunk.Data[reg] = static_cast<uint8_t>(Math::Clamp<int_t>(level, 0, 15));
        Chunk.Mask |= 1 << reg;
      }

      void ChannelBuilder::DisableTone()
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_MIXER] |= (Devices::AYM::DataChunk::REG_MASK_TONEA << Channel);
        Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_MIXER;
      }

      void ChannelBuilder::EnableEnvelope()
      {
        const uint_t reg = Devices::AYM::DataChunk::REG_VOLA + Channel;
        Chunk.Data[reg] |= Devices::AYM::DataChunk::REG_MASK_ENV;
        Chunk.Mask |= 1 << reg;
      }

      void ChannelBuilder::DisableNoise()
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_MIXER] |= (Devices::AYM::DataChunk::REG_MASK_NOISEA << Channel);
        Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_MIXER;
      }

      void TrackBuilder::SetNoise(uint_t level)
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_TONEN] = static_cast<uint8_t>(level & 31);
        Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_TONEN;
      }

      void TrackBuilder::SetEnvelopeType(uint_t type)
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_ENV] = static_cast<uint8_t>(type);
        Chunk.Mask |= 1 << Devices::AYM::DataChunk::REG_ENV;
      }

      void TrackBuilder::SetEnvelopeTone(uint_t tone)
      {
        Chunk.Data[Devices::AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(tone & 0xff);
        Chunk.Data[Devices::AYM::DataChunk::REG_TONEE_H] = static_cast<uint8_t>(tone >> 8);
        Chunk.Mask |= (1 << Devices::AYM::DataChunk::REG_TONEE_L) | (1 << Devices::AYM::DataChunk::REG_TONEE_H);
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

      Renderer::Ptr Chiptune::CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const
      {
        const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
        const AYM::DataIterator::Ptr iterator = CreateDataIterator(trackParams);
        return AYM::CreateRenderer(trackParams, iterator, chip);
      }

      Analyzer::Ptr CreateAnalyzer(Devices::AYM::Device::Ptr device)
      {
        if (Devices::AYM::Chip::Ptr chip = boost::dynamic_pointer_cast<Devices::AYM::Chip>(device))
        {
          return boost::make_shared<AYMAnalyzer>(chip);
        }
        return Analyzer::Ptr();
      }

      DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams, StateIterator::Ptr iterator, DataRenderer::Ptr renderer)
      {
        return boost::make_shared<AYMDataIterator>(trackParams, iterator, renderer);
      }

      Renderer::Ptr CreateRenderer(TrackParameters::Ptr trackParams, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device)
      {
        return boost::make_shared<AYMRenderer>(trackParams, iterator, device);
      }

      Devices::AYM::Receiver::Ptr CreateReceiver(Sound::MultichannelReceiver::Ptr target)
      {
        return boost::make_shared<AYMReceiver>(target);
      }

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
      {
        return boost::make_shared<AYMHolder>(chiptune);
      }
    }
  }
}
