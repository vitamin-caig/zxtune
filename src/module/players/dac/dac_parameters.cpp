/**
 *
 * @file
 *
 * @brief  DAC-based parameters helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/dac/dac_parameters.h"
// common includes
#include <make_ptr.h>
// library includes
#include <core/core_parameters.h>

namespace Module::DAC
{
  class ChipParameters : public Devices::DAC::ChipParameters
  {
  public:
    ChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
      : Samplerate(samplerate)
      , Params(params)
    {}

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint_t BaseSampleFreq() const override
    {
      Parameters::IntType intVal = 0;
      Params->FindValue(Parameters::ZXTune::Core::DAC::SAMPLES_FREQUENCY, intVal);
      return static_cast<uint_t>(intVal);
    }

    uint_t SoundFreq() const override
    {
      return Samplerate;
    }

    bool Interpolate() const override
    {
      Parameters::IntType intVal = Parameters::ZXTune::Core::DAC::INTERPOLATION_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, intVal);
      return intVal != 0;
    }

  private:
    const uint_t Samplerate;
    const Parameters::Accessor::Ptr Params;
  };

  Devices::DAC::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params)
  {
    return MakePtr<ChipParameters>(samplerate, std::move(params));
  }
}  // namespace Module::DAC
