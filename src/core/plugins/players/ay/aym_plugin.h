/**
 *
 * @file
 *
 * @brief  AYM-based player plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "core/plugins/player_plugin.h"
// library includes
#include <formats/chiptune.h>
#include <module/players/aym/aym_factory.h>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::AYM::Factory::Ptr factory);
  PlayerPlugin::Ptr CreateTrackPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                            Module::AYM::Factory::Ptr factory);
  PlayerPlugin::Ptr CreateStreamPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                             Module::AYM::Factory::Ptr factory);
}  // namespace ZXTune
