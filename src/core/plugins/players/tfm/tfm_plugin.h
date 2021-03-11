/**
 *
 * @file
 *
 * @brief  TFM-based player plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "core/plugins/player_plugin.h"
// library includes
#include <formats/chiptune.h>
#include <module/players/tfm/tfm_factory.h>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::TFM::Factory::Ptr factory);
  PlayerPlugin::Ptr CreateTrackPlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder,
                                            Module::TFM::Factory::Ptr factory);
  PlayerPlugin::Ptr CreateStreamPlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder,
                                             Module::TFM::Factory::Ptr factory);
}  // namespace ZXTune
