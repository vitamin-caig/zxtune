/**
 *
 * @file
 *
 * @brief  DAC-related Module properties helper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/dac/dac_properties_helper.h"
// core includes
#include <core/core_parameters.h>

namespace Module::DAC
{
  void PropertiesHelper::SetSamplesFrequency(uint_t freq)
  {
    Delegate.SetValue(Parameters::ZXTune::Core::DAC::SAMPLES_FREQUENCY, freq);
  }
}  // namespace Module::DAC
