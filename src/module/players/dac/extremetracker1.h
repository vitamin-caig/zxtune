/**
 *
 * @file
 *
 * @brief  ExtremeTracker v1 chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/dac/dac_factory.h"

namespace Module::ExtremeTracker1
{
  DAC::Factory::Ptr CreateFactory();
}  // namespace Module::ExtremeTracker1
