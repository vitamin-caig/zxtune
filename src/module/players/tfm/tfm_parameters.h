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

#include "devices/tfm.h"
#include "parameters/accessor.h"
#include "parameters/identifiers_set.h"

namespace Module::TFM
{
  struct ChipParameters
  {
    static const Parameters::IdentifiersSet ALL;

    static Devices::TFM::ChipParameters::Ptr Create(uint_t samplerate, Parameters::Accessor::Ptr params);
  };
}  // namespace Module::TFM
