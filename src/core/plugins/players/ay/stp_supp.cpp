/**
 *
 * @file
 *
 * @brief  Compiled SoundTrackerPro modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include <formats/chiptune/aym/soundtrackerpro.h>
#include <module/players/aym/soundtrackerpro.h>

namespace ZXTune
{
  void RegisterSTPSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::SoundTrackerPro::CreateCompiledModulesDecoder();
    auto factory = Module::SoundTrackerPro::CreateFactory(decoder);
    auto plugin = CreateTrackPlayerPlugin("STP"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
