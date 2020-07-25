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
#include "module/players/aym/aym_base.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <math/numeric.h>
#include <module/players/analyzer.h>
#include <parameters/tracking_helper.h>
#include <sound/mixer_factory.h>

namespace Module
{
  const Debug::Stream Dbg("Core::AYBase");

  class AYMHolder : public AYM::Holder
  {
  public:
    explicit AYMHolder(AYM::Chiptune::Ptr chiptune)
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
      return AYM::CreateRenderer(*this, std::move(params), std::move(target));
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Devices::AYM::Device::Ptr chip) const override
    {
      auto trackParams = AYM::TrackParameters::Create(params);
      auto iterator = Tune->CreateDataIterator(std::move(trackParams));
      auto soundParams = Sound::RenderParameters::Create(std::move(params));
      return AYM::CreateRenderer(std::move(soundParams), std::move(iterator), std::move(chip));
    }

    AYM::Chiptune::Ptr GetChiptune() const override
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
      return AYM::CreateAnalyzer(Device);
    }

    bool RenderFrame() override
    {
      try
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
        LastChunk.TimeStamp = Devices::AYM::Stamp();
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
    const AYM::DataIterator::Ptr Iterator;
    const Devices::AYM::Device::Ptr Device;
    Devices::AYM::DataChunk LastChunk;
    Time::Duration<Devices::AYM::TimeUnit> FrameDuration;
    Sound::LoopParameters Looped;
  };
}

namespace Module
{
  namespace AYM
  {
    Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
    {
      return MakePtr<AYMHolder>(std::move(chiptune));
    }

    Analyzer::Ptr CreateAnalyzer(Devices::AYM::Device::Ptr device)
    {
      if (auto src = std::dynamic_pointer_cast<Devices::StateSource>(device))
      {
        return Module::CreateAnalyzer(std::move(src));
      }
      else
      {
        return Module::CreateStubAnalyzer();
      }
    }

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, AYM::DataIterator::Ptr iterator, Devices::AYM::Device::Ptr device)
    {
      return MakePtr<AYMRenderer>(std::move(params), std::move(iterator), std::move(device));
    }
    
    Devices::AYM::Chip::Ptr CreateChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
    {
      typedef Sound::ThreeChannelsMatrixMixer MixerType;
      auto mixer = MixerType::Create();
      auto pollParams = Sound::CreateMixerNotificationParameters(std::move(params), mixer);
      auto chipParams = AYM::CreateChipParameters(std::move(pollParams));
      return Devices::AYM::CreateChip(std::move(chipParams), std::move(mixer), std::move(target));
    }

    Renderer::Ptr CreateRenderer(const Holder& holder, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
    {
      auto chip = CreateChip(params, std::move(target));
      return holder.CreateRenderer(std::move(params), std::move(chip));
    }
  }
}
