/**
 *
 * @file
 *
 * @brief  ChipTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/chiptracker.h>
#include <module/players/dac/chiptracker.h>

namespace ZXTune
{
  void RegisterCHISupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateChipTrackerDecoder();
    auto factory = Module::ChipTracker::CreateFactory();
    auto plugin = CreatePlayerPlugin("CHI"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
