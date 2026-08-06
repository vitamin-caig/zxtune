/**
 *
 * @file
 *
 * @brief  Compiled SoundTracker modules support
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
  void RegisterSTCSupport(PlayerPluginsRegistrator& registrator)
  {
    using namespace Formats::Chiptune::SoundTracker::Ver1;
    auto decoder = CreateCompiledDecoder();
    auto factory = Module::SoundTracker::CreateFactory(ParseCompiled);
    auto plugin = CreateTrackPlayerPlugin("STC"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
