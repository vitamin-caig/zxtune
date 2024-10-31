/**
 *
 * @file
 *
 * @brief  ProDigiTracker chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/dac/dac_factory.h"

namespace Module::ProDigiTracker
{
  DAC::Factory::Ptr CreateFactory();
}  // namespace Module::ProDigiTracker
