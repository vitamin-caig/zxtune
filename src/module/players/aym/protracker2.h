/**
 *
 * @file
 *
 * @brief  ProTracker2 chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/aym/aym_factory.h"

namespace Module
{
  namespace ProTracker2
  {
    AYM::Factory::Ptr CreateFactory();
  }
}  // namespace Module
