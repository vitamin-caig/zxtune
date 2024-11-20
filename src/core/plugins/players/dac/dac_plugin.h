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

#include "core/plugins/player_plugin.h"
#include "module/players/dac/dac_factory.h"

#include "formats/chiptune.h"

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::DAC::Factory::Ptr factory);
}
