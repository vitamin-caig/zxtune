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
}

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
