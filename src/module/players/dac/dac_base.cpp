/**
* 
* @file
*
* @brief  DAC-based modules support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/dac/dac_base.h"
//common includes
#include <make_ptr.h>
//library includes
#include <module/players/analyzer.h>
#include <parameters/tracking_helper.h>
#include <sound/multichannel_sample.h>

namespace Module
{
  class DACDataIterator : public DAC::DataIterator
  {
  public:
    DACDataIterator(TrackStateIterator::Ptr delegate, DAC::DataRenderer::Ptr renderer)
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

    void GetData(Devices::DAC::Channels& res) const override
    {
      res.assign(CurrentData.begin(), CurrentData.end());
    }
  private:
    void FillCurrentData()
    {
      if (Delegate->IsValid())
      {
        DAC::TrackBuilder builder;
        Render->SynthesizeData(*State, builder);
        builder.GetResult(CurrentData);
      }
      else
      {
        CurrentData.clear();
      }
    }
  private:
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const DAC::DataRenderer::Ptr Render;
    Devices::DAC::Channels CurrentData;
  };

  class DACRenderer : public Renderer
  {
  public:
    DACRenderer(Sound::RenderParameters::Ptr params, DAC::DataIterator::Ptr iterator, Devices::DAC::Chip::Ptr device)
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
      return CreateAnalyzer(Device);
    }

    bool RenderFrame() override
    {
      try
      {
        if (Iterator->IsValid())
        {
          SynchronizeParameters();
          if (LastChunk.TimeStamp == Devices::DAC::Stamp())
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
        LastChunk.TimeStamp = {};
        curFrame = 0;
      }
      if (LastChunk.TimeStamp == Devices::DAC::Stamp())
      {
        Iterator->GetData(LastChunk.Data);
        Device->UpdateState(LastChunk);
      }
      while (curFrame < frameNum && Iterator->IsValid())
      {
        Iterator->NextFrame({});
        LastChunk.TimeStamp += FrameDuration;
        ++curFrame;
        Iterator->GetData(LastChunk.Data);
        Device->UpdateState(LastChunk);
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
      Iterator->GetData(LastChunk.Data);
      Device->RenderData(LastChunk);
    }
  private:
    Parameters::TrackingHelper<Sound::RenderParameters> Params;
    const DAC::DataIterator::Ptr Iterator;
    const Devices::DAC::Chip::Ptr Device;
    Devices::DAC::DataChunk LastChunk;
    Time::Duration<Devices::DAC::TimeUnit> FrameDuration;
    Sound::LoopParameters Looped;
  };
}

namespace Module
{
  namespace DAC
  {
    ChannelDataBuilder TrackBuilder::GetChannel(uint_t chan)
    {
      using namespace Devices::DAC;
      const auto existing = std::find_if(Data.begin(), Data.end(), [chan](const ChannelData& data) {return data.Channel == chan;});
      if (existing != Data.end())
      {
        return ChannelDataBuilder(*existing);
      }
      Data.push_back(ChannelData());
      ChannelData& newOne = Data.back();
      newOne.Channel = chan;
      return ChannelDataBuilder(newOne);
    }

    void TrackBuilder::GetResult(Devices::DAC::Channels& result)
    {
      using namespace Devices::DAC;
      const auto last = std::remove_if(Data.begin(), Data.end(), [](const ChannelData& data) {return data.Mask == 0;});
      result.assign(Data.begin(), last);
    }

    DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
    {
      return MakePtr<DACDataIterator>(std::move(iterator), std::move(renderer));
    }

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DAC::DataIterator::Ptr iterator, Devices::DAC::Chip::Ptr device)
    {
      return MakePtr<DACRenderer>(std::move(params), std::move(iterator), std::move(device));
    }
  }
}
