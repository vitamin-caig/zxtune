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
    TFMRenderer(Time::Microseconds frameDuration, TFM::DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device)
      : Iterator(std::move(iterator))
      , Device(std::move(device))
      , FrameDuration(frameDuration)
    {
    }

    State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return TFM::CreateAnalyzer(Device);
    }

    bool RenderFrame(const Sound::LoopParameters& looped) override
    {
      try
      {
        if (Iterator->IsValid())
        {
          if (LastChunk.TimeStamp == Devices::TFM::Stamp())
          {
            //first chunk
            TransferChunk();
          }
          Iterator->NextFrame(looped);
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
    const Devices::TFM::Device::Ptr Device;
    const Time::Duration<Devices::TFM::TimeUnit> FrameDuration;
    Devices::TFM::DataChunk LastChunk;
  };
}

namespace Module
{
  namespace TFM
  {
    Analyzer::Ptr CreateAnalyzer(Devices::TFM::Device::Ptr device)
    {
      if (auto src = std::dynamic_pointer_cast<Devices::StateSource>(device))
      {
        return Module::CreateAnalyzer(std::move(src));
      }
      return Analyzer::Ptr();
    }

    Renderer::Ptr CreateRenderer(Time::Microseconds frameDuration, DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device)
    {
      return MakePtr<TFMRenderer>(frameDuration, std::move(iterator), std::move(device));
    }
  }
}
