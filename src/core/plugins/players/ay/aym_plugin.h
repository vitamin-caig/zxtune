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

#include "core/plugins/player_plugin.h"
#include "module/players/aym/aym_factory.h"

#include "formats/chiptune.h"

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::AYM::Factory::Ptr factory);
  PlayerPlugin::Ptr CreateTrackPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                            Module::AYM::Factory::Ptr factory);
  PlayerPlugin::Ptr CreateStreamPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                             Module::AYM::Factory::Ptr factory);
}  // namespace ZXTune
