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

#include "core/plugins/player_plugin.h"
#include "module/players/factory.h"

#include "formats/chiptune.h"

#include "types.h"

namespace ZXTune
{
  class PluginId;
}  // namespace ZXTune

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
