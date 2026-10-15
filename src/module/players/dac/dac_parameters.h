/**
 *
 * @file
 *
 * @brief  DAC-based parameters helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "devices/dac.h"
#include "parameters/accessor.h"
#include "parameters/identifiers_set.h"

namespace Module::DAC
{
  struct ChipParameters
  {
    static const Parameters::IdentifiersSet ALL;

    static Devices::DAC::ChipParameters::Ptr Create(uint_t samplerate, Parameters::Accessor::Ptr params);
  };
}  // namespace Module::DAC
