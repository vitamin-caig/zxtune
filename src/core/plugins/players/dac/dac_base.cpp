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
//library includes
#include <core/core_parameters.h>
#include <sound/render_params.h>
#include <sound/sample_convert.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class TrackParametersImpl : public DAC::TrackParameters
  {
  public:
    explicit TrackParametersImpl(Parameters::Accessor::Ptr params)
      : Delegate(Sound::RenderParameters::Create(params))
    {
    }

    virtual bool Looped() const
    {
      return Delegate->Looped();
    }

    virtual Time::Microseconds FrameDuration() const
    {
      return Delegate->FrameDuration();
    }
  private:
    const Sound::RenderParameters::Ptr Delegate;
  };

  template<unsigned Channels>
  class DACReceiver : public Devices::DAC::Receiver
  {
  public:
    explicit DACReceiver(typename Sound::FixedChannelsReceiver<Channels>::Ptr target)
      : Target(target)
    {
    }

    virtual void ApplyData(const Devices::DAC::MultiSoundSample& data)
    {
      assert(data.size() == Channels);
      typename Sound::FixedChannelsSample<Channels>::Type out;
      std::transform(data.begin(), data.end(), out.begin(), &Sound::ToSample<Devices::DAC::SoundSample>);
      Target->ApplyData(out);
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const typename Sound::FixedChannelsReceiver<Channels>::Ptr Target;
  };

  class DACAnalyzer : public Analyzer
  {
  public:
    explicit DACAnalyzer(Devices::DAC::Chip::Ptr device)
      : Device(device)
    {
    }

    virtual void GetState(std::vector<Analyzer::ChannelState>& channels) const
    {
      Devices::DAC::ChannelsState state;
      Device->GetState(state);
      channels.resize(state.size());
      std::transform(state.begin(), state.end(), channels.begin(), &ConvertChannelState);
    }
  private:
    static Analyzer::ChannelState ConvertChannelState(const Devices::DAC::ChanState& in)
    {
      Analyzer::ChannelState out;
      out.Enabled = in.Enabled;
      out.Band = in.Band;
      out.Level = in.LevelInPercents;
      return out;
    }
  private:
    const Devices::DAC::Chip::Ptr Device;
  };

  class ChipParametersImpl : public Devices::DAC::ChipParameters
  {
  public:
    explicit ChipParametersImpl(Parameters::Accessor::Ptr params)
      : Params(params)
      , SoundParams(Sound::RenderParameters::Create(params))
    {
    }

    virtual uint_t SoundFreq() const
    {
      return SoundParams->SoundFreq();
    }

    virtual bool Interpolate() const
    {
      Parameters::IntType intVal = 0;
      Params->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, intVal);
      return intVal != 0;
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const Sound::RenderParameters::Ptr SoundParams;
  };
}

namespace ZXTune
{
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

      TrackParameters::Ptr TrackParameters::Create(Parameters::Accessor::Ptr params)
      {
        return boost::make_shared<TrackParametersImpl>(params);
      }

      Devices::DAC::Receiver::Ptr CreateReceiver(Sound::FixedChannelsReceiver<3>::Ptr target)
      {
        return boost::make_shared<DACReceiver<3> >(target);
      }

      Devices::DAC::Receiver::Ptr CreateReceiver(Sound::FixedChannelsReceiver<4>::Ptr target)
      {
        return boost::make_shared<DACReceiver<4> >(target);
      }

      Analyzer::Ptr CreateAnalyzer(Devices::DAC::Chip::Ptr device)
      {
        return boost::make_shared<DACAnalyzer>(device);
      }

      Devices::DAC::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params)
      {
        return boost::make_shared<ChipParametersImpl>(params);
      }
    }
  }
}
