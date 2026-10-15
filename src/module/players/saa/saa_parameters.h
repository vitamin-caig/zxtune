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

#include "devices/saa.h"
#include "parameters/accessor.h"
#include "parameters/identifiers_set.h"

namespace Module::SAA
{
  struct ChipParameters
  {
    static const Parameters::IdentifiersSet ALL;

    static Devices::SAA::ChipParameters::Ptr Create(uint_t samplerate, Parameters::Accessor::Ptr params);
  };
}  // namespace Module::SAA
