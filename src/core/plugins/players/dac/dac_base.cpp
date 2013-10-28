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
#include <devices/details/parameters_helper.h>
#include <sound/multichannel_sample.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace Module
{
  class DACDataIterator : public DAC::DataIterator
  {
  public:
    DACDataIterator(TrackStateIterator::Ptr delegate, DAC::DataRenderer::Ptr renderer)
      : Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , Render(renderer)
    {
      FillCurrentData();
    }

    virtual void Reset()
    {
      Delegate->Reset();
      Render->Reset();
      FillCurrentData();
    }

    virtual bool IsValid() const
    {
      return Delegate->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      Delegate->NextFrame(looped);
      FillCurrentData();
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return State;
    }

    virtual void GetData(Devices::DAC::Channels& res) const
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

    virtual void Reset()
    {
      Params.Reset();
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = Devices::DAC::Stamp();
      FrameDuration = Devices::DAC::Stamp();
      Looped = false;
    }

    virtual void SetPosition(uint_t frameNum)
    {
      const TrackState::Ptr state = Iterator->GetStateObserver();
      if (state->Frame() > frameNum)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = Devices::DAC::Stamp();
      }
      if (LastChunk.TimeStamp == Devices::DAC::Stamp())
      {
        Iterator->GetData(LastChunk.Data);
        Device->UpdateState(LastChunk);
      }
      while (state->Frame() < frameNum && Iterator->IsValid())
      {
        Iterator->NextFrame(false);
        LastChunk.TimeStamp += FrameDuration;
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
    Devices::Details::ParametersHelper<Sound::RenderParameters> Params;
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
      return boost::make_shared<DACDataIterator>(iterator, renderer);
    }

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DAC::DataIterator::Ptr iterator, Devices::DAC::Chip::Ptr device)
    {
      return boost::make_shared<DACRenderer>(params, iterator, device);
    }
  }
}
