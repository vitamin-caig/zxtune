/**
 *
 * @file
 *
 * @brief  SAA parameters helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <devices/saa.h>
#include <parameters/accessor.h>

namespace Module::SAA
{
  Devices::SAA::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params);
}  // namespace Module::SAA
