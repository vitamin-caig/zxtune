/**
 *
 * @file
 *
 * @brief  SQDigitalTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/sqdigitaltracker.h>
#include <module/players/dac/sqdigitaltracker.h>

namespace ZXTune
{
  void RegisterSQDSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateSQDigitalTrackerDecoder();
    auto factory = Module::SQDigitalTracker::CreateFactory();
    auto plugin = CreatePlayerPlugin("SQD"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
