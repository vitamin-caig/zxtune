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
#include "dac_base.h"
//common includes
#include <make_ptr.h>
//library includes
#include <module/players/analyzer.h>
#include <parameters/tracking_helper.h>
#include <sound/multichannel_sample.h>
//boost includes
#include <boost/bind.hpp>

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

    void NextFrame(bool looped) override
    {
      Delegate->NextFrame(looped);
      FillCurrentData();
    }

    TrackState::Ptr GetStateObserver() const override
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
      for (; Iterator->IsValid(); Iterator->NextFrame(false));
      Iterator->Reset();
#endif
    }

    TrackState::Ptr GetTrackState() const override
    {
      return Iterator->GetStateObserver();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return CreateAnalyzer(Device);
    }

    bool RenderFrame() override
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

    void Reset() override
    {
      Params.Reset();
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = Devices::DAC::Stamp();
      FrameDuration = Devices::DAC::Stamp();
      Looped = false;
    }

    void SetPosition(uint_t frameNum) override
    {
      const TrackState::Ptr state = Iterator->GetStateObserver();
      uint_t curFrame = state->Frame();
      if (curFrame > frameNum)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = Devices::DAC::Stamp();
        curFrame = 0;
      }
      if (LastChunk.TimeStamp == Devices::DAC::Stamp())
      {
        Iterator->GetData(LastChunk.Data);
        Device->UpdateState(LastChunk);
      }
      while (curFrame < frameNum && Iterator->IsValid())
      {
        Iterator->NextFrame(true);
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
    Devices::DAC::Stamp FrameDuration;
    bool Looped;
  };
}

namespace Module
{
  namespace DAC
  {
    ChannelDataBuilder TrackBuilder::GetChannel(uint_t chan)
    {
      using namespace Devices::DAC;
      const std::vector<ChannelData>::iterator existing = std::find_if(Data.begin(), Data.end(),
        boost::bind(&ChannelData::Channel, _1) == chan);
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
      const std::vector<ChannelData>::iterator last = std::remove_if(Data.begin(), Data.end(),
        boost::bind(&ChannelData::Mask, _1) == 0u);
      result.assign(Data.begin(), last);
    }

    DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
    {
      return MakePtr<DACDataIterator>(iterator, renderer);
    }

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DAC::DataIterator::Ptr iterator, Devices::DAC::Chip::Ptr device)
    {
      return MakePtr<DACRenderer>(params, iterator, device);
    }
  }
}
