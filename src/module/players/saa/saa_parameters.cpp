/**
* 
* @file
*
* @brief  SAA parameters helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "saa_parameters.h"
//common includes
#include <make_ptr.h>
//library includes
#include <core/core_parameters.h>
#include <sound/render_params.h>

namespace Module
{
namespace SAA
{
  class ChipParameters : public Devices::SAA::ChipParameters
  {
  public:
    explicit ChipParameters(Parameters::Accessor::Ptr params)
      : Params(params)
      , SoundParams(Sound::RenderParameters::Create(std::move(params)))
    {
    }

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint64_t ClockFreq() const override
    {
      Parameters::IntType val = Parameters::ZXTune::Core::SAA::CLOCKRATE_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Core::SAA::CLOCKRATE, val);
      return val;
    }

    uint_t SoundFreq() const override
    {
      return SoundParams->SoundFreq();
    }

    Devices::SAA::InterpolationType Interpolation() const override
    {
      Parameters::IntType intVal = Parameters::ZXTune::Core::SAA::INTERPOLATION_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Core::SAA::INTERPOLATION, intVal);
      return static_cast<Devices::SAA::InterpolationType>(intVal);
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const Sound::RenderParameters::Ptr SoundParams;
  };

  Devices::SAA::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params)
  {
    return MakePtr<ChipParameters>(std::move(params));
  }
}
}
