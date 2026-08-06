/**
 *
 * @file
 *
 * @brief  TFMMusicMaker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/tfm/tfm_plugin.h"
#include "module/players/tfm/tfmmusicmaker.h"

namespace ZXTune
{
  void RegisterTFESupport(PlayerPluginsRegistrator& registrator)
  {
    {
      using namespace Formats::Chiptune::TFMMusicMaker::Ver05;
      auto decoder = CreateDecoder();
      auto factory = Module::TFMMusicMaker::CreateFactory(Parse);
      auto plugin = CreateTrackPlayerPlugin("TF0"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      using namespace Formats::Chiptune::TFMMusicMaker::Ver13;
      auto decoder = CreateDecoder();
      auto factory = Module::TFMMusicMaker::CreateFactory(Parse);
      auto plugin = CreateTrackPlayerPlugin("TFE"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
