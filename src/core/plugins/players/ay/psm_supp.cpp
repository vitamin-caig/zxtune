/**
 *
 * @file
 *
 * @brief  ProSoundMaker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include "formats/chiptune/aym/prosoundmaker.h"
#include "module/players/aym/prosoundmaker.h"

#include "core/plugin_attrs.h"

#include <utility>

namespace ZXTune
{
  void RegisterPSMSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateProSoundMakerCompiledDecoder();
    auto factory = Module::ProSoundMaker::CreateFactory();
    auto plugin = CreateTrackPlayerPlugin("PSM"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
