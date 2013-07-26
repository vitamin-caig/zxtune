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
        Devices::TFM::DataChunk chunk;
        chunk.TimeStamp = FlushChunk.TimeStamp;
        chunk.Data = Iterator->GetData();
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
