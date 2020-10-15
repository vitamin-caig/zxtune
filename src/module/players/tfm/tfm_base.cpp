/**
* 
* @file
*
* @brief  TFM-based chiptunes common functionality implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/tfm/tfm_base.h"
//common includes
#include <make_ptr.h>
//library includes
#include <module/players/analyzer.h>
#include <sound/loop.h>
//std includes
#include <utility>

namespace Module
{
  class TFMRenderer : public Renderer
  {
  public:
    TFMRenderer(Time::Microseconds frameDuration, TFM::DataIterator::Ptr iterator, Devices::TFM::Chip::Ptr device, Sound::Receiver::Ptr target)
      : Iterator(std::move(iterator))
      , Device(std::move(device))
      , Target(std::move(target))
      , FrameDuration(frameDuration)
    {
    }

    State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return Module::CreateAnalyzer(Device);
    }

    bool RenderFrame(const Sound::LoopParameters& looped) override
    {
      if (Iterator->IsValid())
      {
        TransferChunk();
        Iterator->NextFrame(looped);
        LastChunk.TimeStamp += FrameDuration;
        Target->ApplyData(Device->RenderTill(LastChunk.TimeStamp));
        Target->Flush();
      }
      return Iterator->IsValid();
    }

    void Reset() override
    {
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = {};
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      const auto state = GetState();
      if (request < state->At())
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = {};
      }
      while (state->At() < request && Iterator->IsValid())
      {
        TransferChunk();
        Iterator->NextFrame({});
      }
    }
  private:
    void TransferChunk()
    {
      Iterator->GetData(LastChunk.Data);
      Device->RenderData(LastChunk);
    }
  private:
    const TFM::DataIterator::Ptr Iterator;
    const Devices::TFM::Chip::Ptr Device;
    const Sound::Receiver::Ptr Target;
    const Time::Duration<Devices::TFM::TimeUnit> FrameDuration;
    Devices::TFM::DataChunk LastChunk;
  };
}

namespace Module
{
  namespace TFM
  {
    Renderer::Ptr CreateRenderer(Time::Microseconds frameDuration, DataIterator::Ptr iterator, Devices::TFM::Chip::Ptr device, Sound::Receiver::Ptr target)
    {
      return MakePtr<TFMRenderer>(frameDuration, std::move(iterator), std::move(device), std::move(target));
    }
  }
}
