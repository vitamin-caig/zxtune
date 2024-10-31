/**
 *
 * @file
 *
 * @brief  ProTracker v1.x support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include <formats/chiptune/aym/protracker1.h>
#include <module/players/aym/protracker1.h>

namespace ZXTune
{
  void RegisterPT1Support(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateProTracker1Decoder();
    auto factory = Module::ProTracker1::CreateFactory();
    auto plugin = CreateTrackPlayerPlugin("PT1"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
