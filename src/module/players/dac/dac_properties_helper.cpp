/**
 *
 * @file
 *
 * @brief  DAC-related Module properties helper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/dac/dac_properties_helper.h"

#include "module/players/platforms.h"

#include "core/core_parameters.h"

namespace Module::DAC
{
  PropertiesHelper::PropertiesHelper(Parameters::Modifier& delegate)
    : Module::PropertiesHelper(delegate)
  {
    SetPlatform(Platforms::ZX_SPECTRUM);
  }

  void PropertiesHelper::SetChannels(uint_t count)
  {
    Strings::Array chans;
    for (uint_t ch = 0; ch < count; ++ch)
    {
      chans.emplace_back(1, 'A' + ch);
    }
    Module::PropertiesHelper::SetChannels(chans);
  }

  void PropertiesHelper::SetSamplesFrequency(uint_t freq)
  {
    Delegate.SetValue(Parameters::ZXTune::Core::DAC::SAMPLES_FREQUENCY, freq);
  }
}  // namespace Module::DAC
