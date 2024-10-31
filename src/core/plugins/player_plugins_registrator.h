/**
 *
 * @file
 *
 * @brief  Plugins registrator interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "core/plugins/player_plugin.h"
#include "core/plugins/registrator.h"

namespace ZXTune
{
  class PlayerPluginsRegistrator : public PluginsRegistrator<PlayerPlugin>
  {};
}  // namespace ZXTune
