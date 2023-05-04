/**
 *
 * @file
 *
 * @brief  TFMMusicMaker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/tfm/tfm_plugin.h"
// library includes
#include <module/players/tfm/tfmmusicmaker.h>

namespace ZXTune
{
  void RegisterTFESupport(PlayerPluginsRegistrator& registrator)
  {
    {
      auto decoder = Formats::Chiptune::TFMMusicMaker::Ver05::CreateDecoder();
      auto factory = Module::TFMMusicMaker::CreateFactory(decoder);
      auto plugin = CreateTrackPlayerPlugin("TF0"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = Formats::Chiptune::TFMMusicMaker::Ver13::CreateDecoder();
      auto factory = Module::TFMMusicMaker::CreateFactory(decoder);
      auto plugin = CreateTrackPlayerPlugin("TFE"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
