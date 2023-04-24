/**
 *
 * @file
 *
 * @brief  ExtremeTracker1 support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/extremetracker1.h>
#include <module/players/dac/extremetracker1.h>

namespace ZXTune
{
  void RegisterET1Support(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateExtremeTracker1Decoder();
    auto factory = Module::ExtremeTracker1::CreateFactory();
    auto plugin = CreatePlayerPlugin("ET1"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
