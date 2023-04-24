/**
 *
 * @file
 *
 * @brief  ETracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/saa/etracker.h>
#include <module/players/saa/etracker.h>

namespace ZXTune
{
  void RegisterCOPSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "COP"_id;
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::SAA1099;

    auto decoder = Formats::Chiptune::CreateETrackerDecoder();
    auto factory = Module::ETracker::CreateFactory();
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
