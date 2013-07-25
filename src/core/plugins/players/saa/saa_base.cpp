/*
Abstract:
  SAA-based players common functionality implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "saa_base.h"
#include "core/plugins/players/analyzer.h"
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <devices/details/parameters_helper.h>
#include <math/numeric.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
  class SAADataIterator : public SAA::DataIterator
  {
  public:
    SAADataIterator(TrackStateIterator::Ptr delegate, SAA::DataRenderer::Ptr renderer)
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

    virtual Devices::SAA::DataChunk GetData() const
    {
      return CurrentChunk;
    }
  private:
    void FillCurrentChunk()
    {
      if (Delegate->IsValid())
      {
        SAA::TrackBuilder builder;
        Render->SynthesizeData(*State, builder);
        builder.GetResult(CurrentChunk);
      }
    }
  private:
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const SAA::DataRenderer::Ptr Render;
    Devices::SAA::DataChunk CurrentChunk;
  };

  class SAARenderer : public Renderer
  {
  public:
    SAARenderer(Sound::RenderParameters::Ptr params, SAA::DataIterator::Ptr iterator, Devices::SAA::Device::Ptr device)
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
      return SAA::CreateAnalyzer(Device);
    }

    virtual bool RenderFrame()
    {
      if (Iterator->IsValid())
      {
        SynchronizeParameters();
        Devices::SAA::DataChunk chunk = Iterator->GetData();
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
      FlushChunk = Devices::SAA::DataChunk();
      FrameDuration = Devices::SAA::Stamp();
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

    void CommitChunk(const Devices::SAA::DataChunk& chunk)
    {
      Device->RenderData(chunk);
      FlushChunk.TimeStamp += FrameDuration;
      Device->RenderData(FlushChunk);
      Device->Flush();
    }
  private:
    Devices::Details::ParametersHelper<Sound::RenderParameters> Params;
    const SAA::DataIterator::Ptr Iterator;
    const Devices::SAA::Device::Ptr Device;
    Devices::SAA::DataChunk FlushChunk;
    Devices::SAA::Stamp FrameDuration;
    bool Looped;
  };

  class SAAHolder : public Holder
  {
  public:
    explicit SAAHolder(SAA::Chiptune::Ptr chiptune)
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
      const Devices::SAA::ChipParameters::Ptr chipParams = SAA::CreateChipParameters(params);
      const Devices::SAA::Chip::Ptr chip = Devices::SAA::CreateChip(chipParams, target);
      return Tune->CreateRenderer(params, chip);
    }
  private:
    const SAA::Chiptune::Ptr Tune;
  };
}

namespace Module
{
  namespace SAA
  {
    ChannelBuilder::ChannelBuilder(uint_t chan, Devices::SAA::DataChunk& chunk)
      : Channel(chan)
      , Chunk(chunk)
    {
      SetRegister(Devices::SAA::DataChunk::REG_TONEMIXER, 0, 1 << chan);
      SetRegister(Devices::SAA::DataChunk::REG_NOISEMIXER, 0, 1 << chan);
    }

    void ChannelBuilder::SetVolume(int_t left, int_t right)
    {
      SetRegister(Devices::SAA::DataChunk::REG_LEVEL0 + Channel, 16 * Math::Clamp<int_t>(right, 0, 15) + Math::Clamp<int_t>(left, 0, 15));
    }

    void ChannelBuilder::SetTone(uint_t octave, uint_t note)
    {
      SetRegister(Devices::SAA::DataChunk::REG_TONENUMBER0 + Channel, note);
      AddRegister(Devices::SAA::DataChunk::REG_TONEOCTAVE01 + Channel / 2, 0 != (Channel & 1) ? (octave << 4) : octave);
    }

    void ChannelBuilder::SetNoise(uint_t type)
    {
      const uint_t shift = Channel >= 3 ? 4 : 0;
      SetRegister(Devices::SAA::DataChunk::REG_NOISECLOCK, type << shift, 0x7 << shift);
    }

    void ChannelBuilder::AddNoise(uint_t type)
    {
      const uint_t shift = Channel >= 3 ? 4 : 0;
      AddRegister(Devices::SAA::DataChunk::REG_NOISECLOCK, type << shift);
    }

    void ChannelBuilder::SetEnvelope(uint_t type)
    {
      SetRegister(Devices::SAA::DataChunk::REG_ENVELOPE0 + (Channel >= 3), type);
    }

    void ChannelBuilder::EnableTone()
    {
      AddRegister(Devices::SAA::DataChunk::REG_TONEMIXER, 1 << Channel);
    }

    void ChannelBuilder::EnableNoise()
    {
      AddRegister(Devices::SAA::DataChunk::REG_NOISEMIXER, 1 << Channel);
    }

    Renderer::Ptr Chiptune::CreateRenderer(Parameters::Accessor::Ptr params, Devices::SAA::Device::Ptr chip) const
    {
      const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
      const SAA::DataIterator::Ptr iterator = CreateDataIterator();
      return SAA::CreateRenderer(renderParams, iterator, chip);
    }

    Analyzer::Ptr CreateAnalyzer(Devices::SAA::Device::Ptr device)
    {
      if (Devices::StateSource::Ptr src = boost::dynamic_pointer_cast<Devices::SAA::Chip>(device))
      {
        return Module::CreateAnalyzer(src);
      }
      return Analyzer::Ptr();
    }

    DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
    {
      return boost::make_shared<SAADataIterator>(iterator, renderer);
    }

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::SAA::Device::Ptr device)
    {
      return boost::make_shared<SAARenderer>(params, iterator, device);
    }

    Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
    {
      return boost::make_shared<SAAHolder>(chiptune);
    }
  }
}
