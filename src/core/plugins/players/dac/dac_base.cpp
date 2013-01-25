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

  class DACReceiver : public Devices::DAC::Receiver
  {
  public:
    DACReceiver(Sound::MultichannelReceiver::Ptr target, uint_t channels)
      : Target(target)
      , Data(channels)
    {
    }

    virtual void ApplyData(const Devices::DAC::MultiSoundSample& data)
    {
      std::transform(data.begin(), data.end(), Data.begin(), &Sound::ToSample<Devices::DAC::SoundSample>);
      Target->ApplyData(Data);
    }

    virtual void Flush()
    {
      Target->Flush();
    }
  private:
    const Sound::MultichannelReceiver::Ptr Target;
    Sound::MultichannelSample Data;
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
      TrackParameters::Ptr TrackParameters::Create(Parameters::Accessor::Ptr params)
      {
        return boost::make_shared<TrackParametersImpl>(params);
      }

      Devices::DAC::Receiver::Ptr CreateReceiver(Sound::MultichannelReceiver::Ptr target, uint_t channels)
      {
        return boost::make_shared<DACReceiver>(target, channels);
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
