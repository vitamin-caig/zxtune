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
#include "module/players/aym/ascsoundmaster.h"

namespace ZXTune
{
  void RegisterASCSupport(PlayerPluginsRegistrator& registrator)
  {
    using namespace Formats::Chiptune::ASCSoundMaster;
    {
      auto decoder = Ver0::CreateDecoder();
      auto factory = Module::ASCSoundMaster::CreateFactory(Ver0::Parse);
      auto plugin = CreateTrackPlayerPlugin("AS0"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = Ver1::CreateDecoder();
      auto factory = Module::ASCSoundMaster::CreateFactory(Ver1::Parse);
      auto plugin = CreateTrackPlayerPlugin("ASC"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
