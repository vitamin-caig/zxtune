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

namespace Module
{
  using ExternalParsingFactory = BaseFactory<Formats::Chiptune::Container>;
}

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::Factory::Ptr factory);
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::ExternalParsingFactory::Ptr factory);
}  // namespace ZXTune
