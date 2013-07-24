/*
Abstract:
  DAC-based players common functionality implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dac_base.h"
#include "core/plugins/players/analyzer.h"
//library includes
#include <core/core_parameters.h>
#include <devices/details/parameters_helper.h>
#include <sound/mixer_factory.h>
#include <sound/multichannel_sample.h>
#include <sound/render_params.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class ChipParametersImpl : public Devices::DAC::ChipParameters
  {
  public:
    explicit ChipParametersImpl(Parameters::Accessor::Ptr params)
      : Params(params)
      , SoundParams(Sound::RenderParameters::Create(params))
    {
    }

    virtual uint_t Version() const
    {
      return Params->Version();
    }

    virtual uint_t BaseSampleFreq() const
    {
      Parameters::IntType intVal = 0;
      Params->FindValue(Parameters::ZXTune::Core::DAC::SAMPLES_FREQUENCY, intVal);
      return static_cast<uint_t>(intVal);
    }

    virtual uint_t SoundFreq() const
    {
      return SoundParams->SoundFreq();
    }

    virtual bool Interpolate() const
    {
      Parameters::IntType intVal = 0;
      Params->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, intVal);
      return intVal != 0;
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const Sound::RenderParameters::Ptr SoundParams;
  };

  class DACDataIterator : public DAC::DataIterator
  {
  public:
    DACDataIterator(TrackStateIterator::Ptr delegate, DAC::DataRenderer::Ptr renderer)
      : Delegate(delegate)
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

    virtual void GetData(Devices::DAC::DataChunk& chunk) const
    {
      chunk = CurrentChunk;
    }
  private:
    void FillCurrentChunk()
    {
      if (Delegate->IsValid())
      {
        DAC::TrackBuilder builder;
        Render->SynthesizeData(*State, builder);
        builder.GetResult(CurrentChunk.Channels);
      }
    }
  private:
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const DAC::DataRenderer::Ptr Render;
    Devices::DAC::DataChunk CurrentChunk;
  };

  class DACRenderer : public Renderer
  {
  public:
    DACRenderer(Sound::RenderParameters::Ptr params, DAC::DataIterator::Ptr iterator, Devices::DAC::Chip::Ptr device)
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
      return CreateAnalyzer(Device);
    }

    virtual bool RenderFrame()
    {
      if (Iterator->IsValid())
      {
        SynchronizeParameters();
        Devices::DAC::DataChunk chunk;
        Iterator->GetData(chunk);
        chunk.TimeStamp = FlushChunk.TimeStamp;
        CommitChunk(chunk);
        Iterator->NextFrame(Looped);
      }
      return Iterator->IsValid();
    }

    virtual void Reset()
    {
      Params.Reset();
      Iterator->Reset();
      Device->Reset();
      FlushChunk = Devices::DAC::DataChunk();
      FrameDuration = Devices::DAC::Stamp();
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

    void CommitChunk(const Devices::DAC::DataChunk& chunk)
    {
      Device->RenderData(chunk);
      FlushChunk.TimeStamp += FrameDuration;
      Device->RenderData(FlushChunk);
      Device->Flush();
    }
  private:
    Devices::Details::ParametersHelper<Sound::RenderParameters> Params;
    const DAC::DataIterator::Ptr Iterator;
    const Devices::DAC::Chip::Ptr Device;
    Devices::DAC::DataChunk FlushChunk;
    Devices::DAC::Stamp FrameDuration;
    bool Looped;
  };

  template<unsigned Channels>
  class PollingMixerChip : public Devices::DAC::Chip
  {
  public:
    PollingMixerChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
      : Params(params)
      , Mixer(Sound::FixedChannelsMatrixMixer<Channels>::Create())
      , Delegate(Devices::DAC::CreateChip(DAC::CreateChipParameters(params), Mixer, target))
    {
    }

    virtual void SetSample(uint_t idx, Devices::DAC::Sample::Ptr sample)
    {
      return Delegate->SetSample(idx, sample);
    }

    virtual void RenderData(const Devices::DAC::DataChunk& src)
    {
      return Delegate->RenderData(src);
    }

    virtual void Flush()
    {
      if (Params.IsChanged())
      {
        Sound::FillMixer(*Params, *Mixer);
      }
      return Delegate->Flush();
    }

    virtual void GetState(Devices::MultiChannelState& state) const
    {
      return Delegate->GetState(state);
    }

    virtual void Reset()
    {
      Params.Reset();
      return Delegate->Reset();
    }

    static Devices::DAC::Chip::Ptr Create(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
    {
      return boost::make_shared<PollingMixerChip>(params, target);
    }
  private:
    Devices::Details::ParametersHelper<Parameters::Accessor> Params;
    const typename Sound::FixedChannelsMatrixMixer<Channels>::Ptr Mixer;
    const Devices::DAC::Chip::Ptr Delegate;
  };

  class DACHolder : public Holder
  {
  public:
    explicit DACHolder(DAC::Chiptune::Ptr chiptune)
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
      const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
      const DAC::DataIterator::Ptr iterator = Tune->CreateDataIterator();
      const Devices::DAC::Chip::Ptr chip = CreateChip(params, target);
      Tune->GetSamples(chip);
      return DAC::CreateRenderer(renderParams, iterator, chip);
    }
  private:
    Devices::DAC::Chip::Ptr CreateChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      switch (Tune->GetInformation()->ChannelsCount())
      {
      case 3:
        return PollingMixerChip<3>::Create(params, target);
      case 4:
        return PollingMixerChip<4>::Create(params, target);
      default:
        return Devices::DAC::Chip::Ptr();
      };
    }
  private:
    const DAC::Chiptune::Ptr Tune;
  };
}

namespace ZXTune
{
  namespace Module
  {
    namespace DAC
    {
      ChannelDataBuilder TrackBuilder::GetChannel(uint_t chan)
      {
        using namespace Devices::DAC;
        const std::vector<DataChunk::ChannelData>::iterator existing = std::find_if(Data.begin(), Data.end(),
          boost::bind(&DataChunk::ChannelData::Channel, _1) == chan);
        if (existing != Data.end())
        {
          return ChannelDataBuilder(*existing);
        }
        Data.push_back(DataChunk::ChannelData());
        DataChunk::ChannelData& newOne = Data.back();
        newOne.Channel = chan;
        return ChannelDataBuilder(newOne);
      }

      void TrackBuilder::GetResult(std::vector<Devices::DAC::DataChunk::ChannelData>& result)
      {
        using namespace Devices::DAC;
        const std::vector<DataChunk::ChannelData>::iterator last = std::remove_if(Data.begin(), Data.end(),
          boost::bind(&DataChunk::ChannelData::Mask, _1) == 0u);
        result.assign(Data.begin(), last);
      }

      Devices::DAC::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params)
      {
        return boost::make_shared<ChipParametersImpl>(params);
      }

      DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
      {
        return boost::make_shared<DACDataIterator>(iterator, renderer);
      }

      Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DAC::DataIterator::Ptr iterator, Devices::DAC::Chip::Ptr device)
      {
        return boost::make_shared<DACRenderer>(params, iterator, device);
      }

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
      {
        return boost::make_shared<DACHolder>(chiptune);
      }
    }
  }
}
