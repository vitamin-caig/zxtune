/**
 *
 * @file
 *
 * @brief  FastTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include "formats/chiptune/aym/fasttracker.h"
#include "module/players/aym/fasttracker.h"

namespace ZXTune
{
  void RegisterFTCSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateFastTrackerDecoder();
    auto factory = Module::FastTracker::CreateFactory();
    auto plugin = CreateTrackPlayerPlugin("FTC"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
