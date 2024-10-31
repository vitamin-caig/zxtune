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

#include "formats/chiptune/aym/soundtrackerpro.h"
#include "module/players/aym/aym_factory.h"

namespace Module::SoundTrackerPro
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder);
}  // namespace Module::SoundTrackerPro
