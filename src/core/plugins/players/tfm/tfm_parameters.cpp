/*
Abstract:
  TFM parameters helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "tfm_parameters.h"
//library includes
#include <core/core_parameters.h>
#include <sound/render_params.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class TrackParametersImpl : public TFM::TrackParameters
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

  class ChipParametersImpl : public Devices::TFM::ChipParameters
  {
  public:
    explicit ChipParametersImpl(Parameters::Accessor::Ptr params)
      : Params(params)
      , SoundParams(Sound::RenderParameters::Create(params))
    {
    }

    virtual uint_t Version() const
    {
      return 1;
    }

    virtual uint64_t ClockFreq() const
    {
      Parameters::IntType val = Parameters::ZXTune::Core::FM::CLOCKRATE_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Core::FM::CLOCKRATE, val);
      return val;
    }

    virtual uint_t SoundFreq() const
    {
      return SoundParams->SoundFreq();
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
    namespace TFM
    {
      Devices::TFM::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params)
      {
        return boost::make_shared<ChipParametersImpl>(params);
      }

      TrackParameters::Ptr TrackParameters::Create(Parameters::Accessor::Ptr params)
      {
        return boost::make_shared<TrackParametersImpl>(params);
      }
    }
  }
}
