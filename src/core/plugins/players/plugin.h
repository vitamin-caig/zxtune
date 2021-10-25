/**
 *
 * @file
 *
 * @brief  Player plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "core/plugins/player_plugin.h"
// library includes
#include <formats/chiptune.h>
#include <module/players/factory.h>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(StringView id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::Factory::Ptr factory);
}
