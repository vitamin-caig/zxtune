/**
 *
 * @file
 *
 * @brief  DAC-related Module properties helper interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/properties_helper.h"

namespace Module::DAC
{
  class PropertiesHelper : public Module::PropertiesHelper
  {
  public:
    explicit PropertiesHelper(Parameters::Modifier& delegate);

    void SetSamplesFrequency(uint_t freq);
    void SetChannels(uint_t count);
  };
}  // namespace Module::DAC
