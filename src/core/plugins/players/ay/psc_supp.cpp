/**
 *
 * @file
 *
 * @brief  ProSoundCreator support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include <formats/chiptune/aym/prosoundcreator.h>
#include <module/players/aym/prosoundcreator.h>

namespace ZXTune
{
  void RegisterPSCSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateProSoundCreatorDecoder();
    auto factory = Module::ProSoundCreator::CreateFactory();
    auto plugin = CreateTrackPlayerPlugin("PSC"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
