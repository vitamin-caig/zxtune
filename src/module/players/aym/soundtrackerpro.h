/**
 *
 * @file
 *
 * @brief  SoundTrackerPro-based modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_factory.h"
#include <formats/chiptune/aym/soundtrackerpro.h>

namespace Module::SoundTrackerPro
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder);
}  // namespace Module::SoundTrackerPro
