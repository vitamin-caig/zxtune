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
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <sound/sample_convert.h>
#include <sound/mixer_factory.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG FE68BDBE

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class TFMReceiver : public Devices::TFM::Receiver
  {
  public:
    TFMReceiver(Sound::OneChannelReceiver::Ptr target)
      : Target(target)
    {
    }

    virtual void ApplyData(const Devices::TFM::Sample& data)
    {
      BOOST_STATIC_ASSERT(Sound::OneChannelReceiver::InDataType::static_size == 1);
      Data[0] = Sound::ToSample(data);
      Target->ApplyData(Data);
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const Sound::OneChannelReceiver::Ptr Target;
    Sound::OneChannelReceiver::InDataType Data;
  };

  class TFMAnalyzer : public Analyzer
  {
  public:
    explicit TFMAnalyzer(Devices::TFM::Chip::Ptr device)
      : Device(device)
    {
    }

    virtual void GetState(std::vector<Analyzer::ChannelState>& channels) const
    {
      Devices::TFM::ChannelsState state;
      Device->GetState(state);
      channels.resize(state.size());
      std::transform(state.begin(), state.end(), channels.begin(), &ConvertChannelState);
    }
  private:
    static Analyzer::ChannelState ConvertChannelState(const Devices::TFM::ChanState& in)
    {
      Analyzer::ChannelState out;
      out.Enabled = in.Enabled;
      out.Band = in.Band;
      out.Level = in.LevelInPercents;
      return out;
    }
  private:
    const Devices::TFM::Chip::Ptr Device;
  };

  class TFMRenderer : public Renderer
  {
  public:
    TFMRenderer(TFM::TrackParameters::Ptr params, TFM::DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device)
      : Params(params)
      , Iterator(iterator)
      , Device(device)
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
        Devices::TFM::DataChunk chunk = Iterator->GetData();
        chunk.TimeStamp = FlushChunk.TimeStamp;
        CommitChunk(chunk);
        Iterator->NextFrame(Params->Looped());
      }
      return Iterator->IsValid();
    }

    virtual void Reset()
    {
      Iterator->Reset();
      Device->Reset();
      FlushChunk = Devices::TFM::DataChunk();
    }

    virtual void SetPosition(uint_t frameNum)
    {
      SeekIterator(*Iterator, frameNum);
    }
  private:
    void CommitChunk(const Devices::TFM::DataChunk& chunk)
    {
      Device->RenderData(chunk);
      FlushChunk.TimeStamp += Params->FrameDuration();
      Device->RenderData(FlushChunk);
      Device->Flush();
    }
  private:
    const TFM::TrackParameters::Ptr Params;
    const TFM::DataIterator::Ptr Iterator;
    const Devices::TFM::Device::Ptr Device;
    Devices::TFM::DataChunk FlushChunk;
  };

  class TFMHolder : public Holder
  {
  public:
    explicit TFMHolder(TFM::Chiptune::Ptr chiptune)
      : Tune(chiptune)
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Tune->GetInformation();
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Tune->GetProperties();
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      const Sound::OneChannelMixer::Ptr mixer = Sound::CreateOneChannelMixer(params);
      mixer->SetTarget(target);
      const Devices::TFM::Receiver::Ptr receiver = TFM::CreateReceiver(mixer);
      const Devices::TFM::ChipParameters::Ptr chipParams = TFM::CreateChipParameters(params);
      const Devices::TFM::Chip::Ptr chip = Devices::TFM::CreateChip(chipParams, receiver);
      return Tune->CreateRenderer(params, chip);
    }
  private:
    const TFM::Chiptune::Ptr Tune;
  };
}

namespace ZXTune
{
  namespace Module
  {
    namespace TFM
    {
      Renderer::Ptr Chiptune::CreateRenderer(Parameters::Accessor::Ptr params, Devices::TFM::Device::Ptr chip) const
      {
        const TFM::TrackParameters::Ptr trackParams = TFM::TrackParameters::Create(params);
        const TFM::DataIterator::Ptr iterator = CreateDataIterator(trackParams);
        return TFM::CreateRenderer(trackParams, iterator, chip);
      }

      Analyzer::Ptr CreateAnalyzer(Devices::TFM::Device::Ptr device)
      {
        if (Devices::TFM::Chip::Ptr chip = boost::dynamic_pointer_cast<Devices::TFM::Chip>(device))
        {
          return boost::make_shared<TFMAnalyzer>(chip);
        }
        return Analyzer::Ptr();
      }

      Renderer::Ptr CreateRenderer(TrackParameters::Ptr trackParams, DataIterator::Ptr iterator, Devices::TFM::Device::Ptr device)
      {
        return boost::make_shared<TFMRenderer>(trackParams, iterator, device);
      }

      Devices::TFM::Receiver::Ptr CreateReceiver(Sound::OneChannelReceiver::Ptr target)
      {
        return boost::make_shared<TFMReceiver>(target);
      }

      Holder::Ptr CreateHolder(Chiptune::Ptr chiptune)
      {
        return boost::make_shared<TFMHolder>(chiptune);
      }
    }
  }
}
