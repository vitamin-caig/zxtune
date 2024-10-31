/**
 *
 * @file
 *
 * @brief  SoundTracker-based modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_factory.h"
#include <formats/chiptune/aym/soundtracker.h>

namespace Module::SoundTracker
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::SoundTracker::Decoder::Ptr decoder);
}  // namespace Module::SoundTracker
