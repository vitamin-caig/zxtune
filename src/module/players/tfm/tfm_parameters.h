/**
 *
 * @file
 *
 * @brief  TFM parameters helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <devices/tfm.h>
#include <parameters/accessor.h>

namespace Module::TFM
{
  Devices::TFM::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params);
}  // namespace Module::TFM
