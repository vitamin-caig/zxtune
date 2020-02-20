/**
* 
* @file
*
* @brief  SAA-based chiptunes common functionality implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/saa/saa_base.h"
//common includes
#include <make_ptr.h>
//library includes
#include <math/numeric.h>
#include <module/players/analyzer.h>
#include <parameters/tracking_helper.h>
//std includes
#include <utility>

namespace Module
{
  class SAADataIterator : public SAA::DataIterator
  {
  public:
    SAADataIterator(TrackStateIterator::Ptr delegate, SAA::DataRenderer::Ptr renderer)
      : Delegate(std::move(delegate))
      , State(Delegate->GetStateObserver())
      , Render(std::move(renderer))
    {
      FillCurrentData();
    }

    void Reset() override
    {
      Delegate->Reset();
      Render->Reset();
      FillCurrentData();
    }

    bool IsValid() const override
    {
      return Delegate->IsValid();
    }

    void NextFrame(const Sound::LoopParameters& looped) override
    {
      Delegate->NextFrame(looped);
      FillCurrentData();
    }

    Module::State::Ptr GetStateObserver() const override
    {
      return State;
    }

    Devices::SAA::Registers GetData() const override
    {
      return CurrentData;
    }
  private:
    void FillCurrentData()
    {
      if (Delegate->IsValid())
      {
        SAA::TrackBuilder builder;
        Render->SynthesizeData(*State, builder);
        builder.GetResult(CurrentData);
      }
    }
  private:
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const SAA::DataRenderer::Ptr Render;
    Devices::SAA::Registers CurrentData;
  };

  class SAARenderer : public Renderer
  {
  public:
    SAARenderer(Sound::RenderParameters::Ptr params, SAA::DataIterator::Ptr iterator, Devices::SAA::Device::Ptr device)
      : Params(std::move(params))
      , Iterator(std::move(iterator))
      , Device(std::move(device))
      , FrameDuration()
      , Looped()
    {
#ifndef NDEBUG
//perform self-test
      for (; Iterator->IsValid(); Iterator->NextFrame({}));
      Iterator->Reset();
#endif
    }

    State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return SAA::CreateAnalyzer(Device);
    }

    bool RenderFrame() override
    {
      try
      {
        if (Iterator->IsValid())
        {
          SynchronizeParameters();
          if (LastChunk.TimeStamp == Devices::SAA::Stamp())
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
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      Params.Reset();
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = {};
      FrameDuration = {};
      Looped = {};
    }

    void SetPosition(uint_t frameNum) override
    {
      uint_t curFrame = GetState()->Frame();
      if (curFrame > frameNum)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = Devices::SAA::Stamp();
        curFrame = 0;
      }
      while (curFrame < frameNum && Iterator->IsValid())
      {
        TransferChunk();
        Iterator->NextFrame({});
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
    const SAA::DataIterator::Ptr Iterator;
    const Devices::SAA::Device::Ptr Device;
    Devices::SAA::DataChunk LastChunk;
    Time::Duration<Devices::SAA::TimeUnit> FrameDuration;
    Sound::LoopParameters Looped;
  };

  class SAAHolder : public Holder
  {
  public:
    explicit SAAHolder(SAA::Chiptune::Ptr chiptune)
      : Tune(std::move(chiptune))
    {
    }

    Information::Ptr GetModuleInformation() const override
    {
      return Tune->GetInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Tune->GetProperties();
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      auto chipParams = SAA::CreateChipParameters(params);
      auto chip = Devices::SAA::CreateChip(std::move(chipParams), std::move(target));
      auto renderParams = Sound::RenderParameters::Create(std::move(params));
      auto iterator = Tune->CreateDataIterator();
      return SAA::CreateRenderer(std::move(renderParams), std::move(iterator), std::move(chip));
    }
  private:
    const SAA::Chiptune::Ptr Tune;
  };
}

namespace Module
{
  namespace SAA
  {
    ChannelBuilder::ChannelBuilder(uint_t chan, Devices::SAA::Registers& regs)
      : Channel(chan)
      , Regs(regs)
    {
      SetRegister(Devices::SAA::Registers::TONEMIXER, 0, 1 << chan);
      SetRegister(Devices::SAA::Registers::NOISEMIXER, 0, 1 << chan);
    }

    void ChannelBuilder::SetVolume(int_t left, int_t right)
    {
      SetRegister(Devices::SAA::Registers::LEVEL0 + Channel, 16 * Math::Clamp<int_t>(right, 0, 15) + Math::Clamp<int_t>(left, 0, 15));
    }

    void ChannelBuilder::SetTone(uint_t octave, uint_t note)
    {
      SetRegister(Devices::SAA::Registers::TONENUMBER0 + Channel, note);
      AddRegister(Devices::SAA::Registers::TONEOCTAVE01 + Channel / 2, 0 != (Channel & 1) ? (octave << 4) : octave);
    }

    void ChannelBuilder::SetNoise(uint_t type)
    {
      const uint_t shift = Channel >= 3 ? 4 : 0;
      SetRegister(Devices::SAA::Registers::NOISECLOCK, type << shift, 0x7 << shift);
    }

    void ChannelBuilder::AddNoise(uint_t type)
    {
      const uint_t shift = Channel >= 3 ? 4 : 0;
      AddRegister(Devices::SAA::Registers::NOISECLOCK, type << shift);
    }

    void ChannelBuilder::SetEnvelope(uint_t type)
    {
      SetRegister(Devices::SAA::Registers::ENVELOPE0 + (Channel >= 3), type);
    }

    void ChannelBuilder::EnableTone()
    {
      AddRegister(Devices::SAA::Registers::TONEMIXER, 1 << Channel);
    }

    void ChannelBuilder::EnableNoise()
    {
      AddRegister(Devices::SAA::Registers::NOISEMIXER, 1 << Channel);
    }

    Analyzer::Ptr CreateAnalyzer(Devices::SAA::Device::Ptr device)
    {
      if (auto src = std::dynamic_pointer_cast<Devices::SAA::Chip>(device))
      {
        return Module::CreateAnalyzer(std::move(src));
      }
      return Analyzer::Ptr();
    }

    DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
    {
      return MakePtr<SAADataIterator>(std::move(iterator), std::move(renderer));
    }

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::SAA::Device::Ptr device)
    {
      return MakePtr<SAARenderer>(std::move(params), std::move(iterator), std::move(device));
    }

    Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
    {
      return MakePtr<SAAHolder>(std::move(chiptune));
    }
  }
}
