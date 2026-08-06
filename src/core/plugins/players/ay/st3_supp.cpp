/**
 *
 * @file
 *
 * @brief  SoundTracker v3 modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include "formats/chiptune/aym/soundtracker.h"
#include "module/players/aym/soundtracker.h"

namespace ZXTune
{
  void RegisterST3Support(PlayerPluginsRegistrator& registrator)
  {
    using namespace Formats::Chiptune::SoundTracker::Ver3;
    auto decoder = CreateDecoder();
    auto factory = Module::SoundTracker::CreateFactory(Parse);
    auto plugin = CreateTrackPlayerPlugin("ST3"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
