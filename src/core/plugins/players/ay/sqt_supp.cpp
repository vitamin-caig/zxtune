/**
 *
 * @file
 *
 * @brief  SQTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include <formats/chiptune/aym/sqtracker.h>
#include <module/players/aym/sqtracker.h>

namespace ZXTune
{
  void RegisterSQTSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateSQTrackerDecoder();
    auto factory = Module::SQTracker::CreateFactory();
    auto plugin = CreateTrackPlayerPlugin("SQT"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
