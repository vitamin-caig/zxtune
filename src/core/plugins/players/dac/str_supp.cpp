/**
 *
 * @file
 *
 * @brief  SampleTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
#include <formats/chiptune/digital/sampletracker.h>
#include <module/players/dac/sampletracker.h>

#include <core/plugin_attrs.h>

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
