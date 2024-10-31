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

#include "formats/chiptune/aym/soundtracker.h"
#include "module/players/aym/aym_factory.h"

namespace Module::SoundTracker
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::SoundTracker::Decoder::Ptr decoder);
}  // namespace Module::SoundTracker
