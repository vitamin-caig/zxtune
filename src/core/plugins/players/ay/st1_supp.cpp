/**
 *
 * @file
 *
 * @brief  Uncompiled SoundTracker modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
// library includes
#include <formats/chiptune/aym/soundtracker.h>
#include <module/players/aym/soundtracker.h>

namespace ZXTune
{
  void RegisterST1Support(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::SoundTracker::Ver1::CreateUncompiledDecoder();
    auto factory = Module::SoundTracker::CreateFactory(decoder);
    auto plugin = CreateTrackPlayerPlugin("ST1"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
