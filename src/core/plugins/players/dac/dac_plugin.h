/**
 *
 * @file
 *
 * @brief  DAC-based player plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "core/plugins/player_plugin.h"
// library includes
#include <formats/chiptune.h>
#include <module/players/dac/dac_factory.h>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::DAC::Factory::Ptr factory);
}
