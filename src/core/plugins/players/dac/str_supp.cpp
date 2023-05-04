/**
 *
 * @file
 *
 * @brief  SampleTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/
// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/sampletracker.h>
#include <module/players/dac/sampletracker.h>

namespace ZXTune
{
  void RegisterSTRSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateSampleTrackerDecoder();
    auto factory = Module::SampleTracker::CreateFactory();
    auto plugin = CreatePlayerPlugin("STR"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
