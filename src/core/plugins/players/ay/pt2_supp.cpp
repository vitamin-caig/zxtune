/**
 *
 * @file
 *
 * @brief  ProTracker v2.x support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include "formats/chiptune/aym/protracker2.h"
#include "module/players/aym/protracker2.h"

namespace ZXTune
{
  void RegisterPT2Support(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateProTracker2Decoder();
    auto factory = Module::ProTracker2::CreateFactory();
    auto plugin = CreateTrackPlayerPlugin("PT2"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
