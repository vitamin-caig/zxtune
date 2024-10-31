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

#include <devices/dac.h>
#include <parameters/accessor.h>

namespace Module::DAC
{
  Devices::DAC::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params);
}  // namespace Module::DAC
