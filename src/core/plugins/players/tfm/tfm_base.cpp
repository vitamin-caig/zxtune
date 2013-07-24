/*
Abstract:
  TFM-based players common functionality implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "tfm_base.h"
#include "core/plugins/players/analyzer.h"
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <devices/details/parameters_helper.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class TFMDataIterator : public TFM::DataIterator
  {
  public:
    TFMDataIterator(TrackStateIterator::Ptr delegate, TFM::DataRenderer::Ptr renderer)
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

    virtual Devices::TFM::DataChunk GetData() const
    {
      return CurrentChunk;
    }
  private:
    void FillCurrentChunk()
    {
      if (Delegate->IsValid())
      {
        TFM::TrackBuilder builder;
        Render->SynthesizeData(*State, builder);
        builder.GetResult(CurrentChunk);
      }
    }
  private:
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const TFM::DataRenderer::Ptr Render;
    Devices::TFM::DataChunk CurrentChunk;
  };


  class TFMRenderer : public Renderer
  {
  public:
    TFMRenderer(Sound::RenderParameters::Ptr params, TFM::DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device)
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
      return TFM::CreateAnalyzer(Device);
    }

    virtual bool RenderFrame()
    {
      if (Iterator->IsValid())
      {
        SynchronizeParameters();
        Devices::TFM::DataChunk chunk = Iterator->GetData();
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
      FlushChunk = Devices::TFM::DataChunk();
      FrameDuration = Devices::TFM::Stamp();
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

    void CommitChunk(const Devices::TFM::DataChunk& chunk)
    {
      Device->RenderData(chunk);
      FlushChunk.TimeStamp += FrameDuration;
      Device->RenderData(FlushChunk);
      Device->Flush();
    }
  private:
    Devices::Details::ParametersHelper<Sound::RenderParameters> Params;
    const TFM::DataIterator::Ptr Iterator;
    const Devices::TFM::Device::Ptr Device;
    Devices::TFM::DataChunk FlushChunk;
    Devices::TFM::Stamp FrameDuration;
    bool Looped;
  };

  class TFMHolder : public Holder
  {
  public:
    explicit TFMHolder(TFM::Chiptune::Ptr chiptune)
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
      const Devices::TFM::ChipParameters::Ptr chipParams = TFM::CreateChipParameters(params);
      const Devices::TFM::Chip::Ptr chip = Devices::TFM::CreateChip(chipParams, target);
      return Tune->CreateRenderer(params, chip);
    }
  private:
    const TFM::Chiptune::Ptr Tune;
  };

  inline uint_t EncodeDetune(int_t in)
  {
    if (in >= 0)
    {
      return in;
    }
    else
    {
      return 4 - in;
    }
  }
}

namespace ZXTune
{
  namespace Module
  {
    namespace TFM
    {
      ChannelBuilder::ChannelBuilder(uint_t chan, Devices::TFM::DataChunk& chunk)
        : Channel(chan >= TFM::TRACK_CHANNELS / 2 ? chan - TFM::TRACK_CHANNELS / 2 : chan)
        , Registers(chunk.Data[chan >= TFM::TRACK_CHANNELS / 2])
      {
      }

      void ChannelBuilder::SetMode(uint_t mode)
      {
        WriteChipRegister(0x27, mode);
      }

      void ChannelBuilder::KeyOn()
      {
        SetKey(0xf);
      }

      void ChannelBuilder::KeyOff()
      {
        SetKey(0);
      }

      void ChannelBuilder::SetKey(uint_t mask)
      {
        WriteChipRegister(0x28, Channel | (mask << 4));
      }

      void ChannelBuilder::SetupConnection(uint_t algorithm, uint_t feedback)
      {
        const uint_t val = algorithm | (feedback << 3);
        WriteChannelRegister(0xb0, val);
      }

      void ChannelBuilder::SetDetuneMultiple(uint_t op, int_t detune, uint_t multiple)
      {
        const uint_t val = (EncodeDetune(detune) << 4) | multiple;
        WriteOperatorRegister(0x30, op, val);
      }

      void ChannelBuilder::SetRateScalingAttackRate(uint_t op, uint_t rateScaling, uint_t attackRate)
      {
        const uint_t val = (rateScaling << 6) | attackRate;
        WriteOperatorRegister(0x50, op, val);
      }

      void ChannelBuilder::SetDecay(uint_t op, uint_t decay)
      {
        WriteOperatorRegister(0x60, op, decay);
      }

      void ChannelBuilder::SetSustain(uint_t op, uint_t sustain)
      {
        WriteOperatorRegister(0x70, op, sustain);
      }

      void ChannelBuilder::SetSustainLevelReleaseRate(uint_t op, uint_t sustainLevel, uint_t releaseRate)
      {
        const uint_t val = (sustainLevel << 4) | releaseRate;
        WriteOperatorRegister(0x80, op, val);
      }

      void ChannelBuilder::SetEnvelopeType(uint_t op, uint_t type)
      {
        WriteOperatorRegister(0x90, op, type);
      }

      void ChannelBuilder::SetTotalLevel(uint_t op, uint_t totalLevel)
      {
        WriteOperatorRegister(0x40, op, totalLevel);
      }

      void ChannelBuilder::SetTone(uint_t octave, uint_t tone)
      {
        const uint_t valHi = octave * 8 + (tone >> 8);
        const uint_t valLo = tone & 0xff;
        WriteChannelRegister(0xa4, valHi);
        WriteChannelRegister(0xa0, valLo);
      }

      void ChannelBuilder::SetTone(uint_t op, uint_t octave, uint_t tone)
      {
        const uint_t valHi = octave * 8 + (tone >> 8);
        const uint_t valLo = tone & 0xff;
        assert(Channel == 2);
        switch (op)
        {
        case 0:
          WriteChipRegister(0xa4, valHi);
          WriteChipRegister(0xa0, valLo);
          break;
        case 1:
          WriteChipRegister(0xac, valHi);
          WriteChipRegister(0xa8, valLo);
          break;
        case 2:
          WriteChipRegister(0xae, valHi);
          WriteChipRegister(0xaa, valLo);
          break;
        case 3:
          //op1 in doc???
          WriteChipRegister(0xad, valHi);
          WriteChipRegister(0xa9, valLo);
          break;
        }
      }

      void ChannelBuilder::SetPane(uint_t val)
      {
        WriteChannelRegister(0xb4, val);
      }

      void ChannelBuilder::WriteOperatorRegister(uint_t base, uint_t op, uint_t val)
      {
        WriteChannelRegister(base + 4 * op, val);
      }

      void ChannelBuilder::WriteChannelRegister(uint_t base, uint_t val)
      {
        Registers.push_back(Devices::FM::DataChunk::Register(base + Channel, val));
      }

      void ChannelBuilder::WriteChipRegister(uint_t idx, uint_t val)
      {
        const Devices::FM::DataChunk::Register reg(idx, val);
        Registers.push_back(reg);
      }

      Renderer::Ptr Chiptune::CreateRenderer(Parameters::Accessor::Ptr params, Devices::TFM::Device::Ptr chip) const
      {
        const Sound::RenderParameters::Ptr soundParams = Sound::RenderParameters::Create(params);
        const TFM::DataIterator::Ptr iterator = CreateDataIterator();
        return TFM::CreateRenderer(soundParams, iterator, chip);
      }

      Analyzer::Ptr CreateAnalyzer(Devices::TFM::Device::Ptr device)
      {
        if (Devices::StateSource::Ptr src = boost::dynamic_pointer_cast<Devices::StateSource>(device))
        {
          return Module::CreateAnalyzer(src);
        }
        return Analyzer::Ptr();
      }

      DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
      {
        return boost::make_shared<TFMDataIterator>(iterator, renderer);
      }

      Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device)
      {
        return boost::make_shared<TFMRenderer>(params, iterator, device);
      }

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
      {
        return boost::make_shared<TFMHolder>(chiptune);
      }
    }
  }
}
