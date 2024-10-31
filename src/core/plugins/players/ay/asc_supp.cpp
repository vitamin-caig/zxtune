/**
 *
 * @file
 *
 * @brief  ASCSoundMaster support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include "formats/chiptune/aym/ascsoundmaster.h"
#include "module/players/aym/ascsoundmaster.h"

#include "core/plugin_attrs.h"

#include <utility>

namespace ZXTune
{
  void RegisterASCSupport(PlayerPluginsRegistrator& registrator)
  {
    {
      auto decoder = Formats::Chiptune::ASCSoundMaster::Ver0::CreateDecoder();
      auto factory = Module::ASCSoundMaster::CreateFactory(decoder);
      auto plugin = CreateTrackPlayerPlugin("AS0"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = Formats::Chiptune::ASCSoundMaster::Ver1::CreateDecoder();
      auto factory = Module::ASCSoundMaster::CreateFactory(decoder);
      auto plugin = CreateTrackPlayerPlugin("ASC"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
