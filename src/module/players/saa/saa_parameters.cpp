/**
 *
 * @file
 *
 * @brief  SAA parameters helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/saa/saa_parameters.h"
// common includes
#include <make_ptr.h>
// library includes
#include <core/core_parameters.h>
// std includes
#include <utility>

namespace Module::SAA
{
  class ChipParameters : public Devices::SAA::ChipParameters
  {
  public:
    ChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
      : Samplerate(samplerate)
      , Params(std::move(params))
    {}

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
      return Samplerate;
    }

    Devices::SAA::InterpolationType Interpolation() const override
    {
      Parameters::IntType intVal = Parameters::ZXTune::Core::SAA::INTERPOLATION_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Core::SAA::INTERPOLATION, intVal);
      return static_cast<Devices::SAA::InterpolationType>(intVal);
    }

  private:
    const uint_t Samplerate;
    const Parameters::Accessor::Ptr Params;
  };

  Devices::SAA::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    return MakePtr<ChipParameters>(samplerate, std::move(params));
  }
}  // namespace Module::SAA
