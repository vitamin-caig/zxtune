/**
 *
 * @file
 *
 * @brief  ETracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/saa/etracker.h"
#include "module/players/saa/etracker.h"

#include "core/plugin_attrs.h"

#include "types.h"

#include <utility>

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
