/**
* 
* @file
*
* @brief  MIDI-based chiptunes common functionality implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "midi_base.h"
#include "core/plugins/players/analyzer.h"
//library includes
#include <devices/details/parameters_helper.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
  class MIDIRenderer : public Renderer
  {
  public:
    MIDIRenderer(Sound::RenderParameters::Ptr params, MIDI::DataIterator::Ptr iterator, Devices::MIDI::Device::Ptr device)
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
      return MIDI::CreateAnalyzer(Device);
    }

    virtual bool RenderFrame()
    {
      if (Iterator->IsValid())
      {
        SynchronizeParameters();
        if (LastChunk.TimeStamp == Devices::MIDI::Stamp())
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
      LastChunk.TimeStamp = Devices::MIDI::Stamp();
      FrameDuration = Devices::MIDI::Stamp();
      Looped = false;
    }

    virtual void SetPosition(uint_t frameNum)
    {
      const TrackState::Ptr state = Iterator->GetStateObserver();
      if (state->Frame() > frameNum)
      {
        Iterator->Reset();
        Device->Reset();
        LastChunk.TimeStamp = Devices::MIDI::Stamp();
      }
      while (state->Frame() < frameNum && Iterator->IsValid())
      {
        Iterator->GetData(LastChunk);
        Device->UpdateState(LastChunk);
        Iterator->NextFrame(false);
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
      Iterator->GetData(LastChunk);
      Device->RenderData(LastChunk);
    }
  private:
    Devices::Details::ParametersHelper<Sound::RenderParameters> Params;
    const MIDI::DataIterator::Ptr Iterator;
    const Devices::MIDI::Device::Ptr Device;
    Devices::MIDI::DataChunk LastChunk;
    Devices::MIDI::Stamp FrameDuration;
    bool Looped;
  };
}

namespace Module
{
  namespace MIDI
  {
    Analyzer::Ptr CreateAnalyzer(Devices::MIDI::Device::Ptr device)
    {
      if (Devices::StateSource::Ptr src = boost::dynamic_pointer_cast<Devices::StateSource>(device))
      {
        return Module::CreateAnalyzer(src);
      }
      return Analyzer::Ptr();
    }

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::MIDI::Device::Ptr device)
    {
      return boost::make_shared<MIDIRenderer>(params, iterator, device);
    }
  }
}
