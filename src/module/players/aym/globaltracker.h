/**
 *
 * @file
 *
 * @brief  GlobalTracker chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_factory.h"

namespace Module::GlobalTracker
{
  AYM::Factory::Ptr CreateFactory();
}  // namespace Module::GlobalTracker
