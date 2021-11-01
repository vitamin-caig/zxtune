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

#include "player_plugin.h"
#include "registrator.h"

namespace ZXTune
{
  class PlayerPluginsRegistrator : public PluginsRegistrator<PlayerPlugin>
  {};
}  // namespace ZXTune
