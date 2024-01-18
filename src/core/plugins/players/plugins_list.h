/**
 *
 * @file
 *
 * @brief  Player plugins factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

namespace ZXTune
{
  class PlayerPluginsRegistrator;
  class ArchivePluginsRegistrator;

  void RegisterPlayerPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives);
}  // namespace ZXTune
