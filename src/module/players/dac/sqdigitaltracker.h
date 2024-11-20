/**
 *
 * @file
 *
 * @brief  SQDigitalTracker chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/dac/dac_factory.h"

namespace Module::SQDigitalTracker
{
  DAC::Factory::Ptr CreateFactory();
}  // namespace Module::SQDigitalTracker
