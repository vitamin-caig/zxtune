/**
 *
 * @file
 *
 * @brief  GlobalTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include "formats/chiptune/aym/globaltracker.h"
#include "module/players/aym/globaltracker.h"

#include "core/plugin_attrs.h"

#include <utility>

namespace ZXTune
{
  void RegisterGTRSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateGlobalTrackerDecoder();
    auto factory = Module::GlobalTracker::CreateFactory();
    auto plugin = CreateTrackPlayerPlugin("GTR"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
