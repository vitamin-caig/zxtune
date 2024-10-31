/**
 *
 * @file
 *
 * @brief  TurboFM Dump support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/tfm/tfm_plugin.h"
#include <formats/chiptune/fm/tfd.h>
#include <module/players/tfm/tfd.h>

namespace ZXTune
{
  void RegisterTFDSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateTFDDecoder();
    auto factory = Module::TFD::CreateFactory();
    auto plugin = CreateStreamPlayerPlugin("TFD"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
