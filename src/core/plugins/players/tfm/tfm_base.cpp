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
#include "tfm_base.h"
#include "core/plugins/players/analyzer.h"
//library includes
#include <devices/details/parameters_helper.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
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
        if (LastChunk.TimeStamp == Devices::TFM::Stamp())
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
      LastChunk.TimeStamp = Devices::TFM::Stamp();
      FrameDuration = Devices::TFM::Stamp();
      Looped = false;
    }

    virtual void SetPosition(uint_t frameNum)
    {
      const TrackState::Ptr state = Iterator->GetStateObserver();
      uint_t curFrame = state->Frame();
      if (curFrame > frameNum)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = Devices::TFM::Stamp();
        curFrame = 0;
      }
      while (curFrame < frameNum && Iterator->IsValid())
      {
        TransferChunk();
        Iterator->NextFrame(true);
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
      Iterator->GetData(LastChunk.Data);
      Device->RenderData(LastChunk);
    }
  private:
    Devices::Details::ParametersHelper<Sound::RenderParameters> Params;
    const TFM::DataIterator::Ptr Iterator;
    const Devices::TFM::Device::Ptr Device;
    Devices::TFM::DataChunk LastChunk;
    Devices::TFM::Stamp FrameDuration;
    bool Looped;
  };
}

namespace Module
{
  namespace TFM
  {
    Analyzer::Ptr CreateAnalyzer(Devices::TFM::Device::Ptr device)
    {
      if (Devices::StateSource::Ptr src = boost::dynamic_pointer_cast<Devices::StateSource>(device))
      {
        return Module::CreateAnalyzer(src);
      }
      return Analyzer::Ptr();
    }

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device)
    {
      return boost::make_shared<TFMRenderer>(params, iterator, device);
    }
  }
}
