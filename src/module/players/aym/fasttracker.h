/**
 *
 * @file
 *
 * @brief  FastTracker chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_factory.h"

namespace Module::FastTracker
{
  AYM::Factory::Ptr CreateFactory();
}  // namespace Module::FastTracker
