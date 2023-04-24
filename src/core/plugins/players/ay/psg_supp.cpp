/**
 *
 * @file
 *
 * @brief  PSG support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
// library includes
#include <formats/chiptune/aym/psg.h>
#include <module/players/aym/psg.h>

namespace ZXTune
{
  void RegisterPSGSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreatePSGDecoder();
    auto factory = Module::PSG::CreateFactory();
    auto plugin = CreateStreamPlayerPlugin("PSG"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
