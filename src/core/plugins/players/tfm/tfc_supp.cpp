/**
 *
 * @file
 *
 * @brief  TurboFM Compiled support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/tfm/tfm_plugin.h"
#include <formats/chiptune/fm/tfc.h>
#include <module/players/tfm/tfc.h>

#include <core/plugin_attrs.h>

namespace ZXTune
{
  void RegisterTFCSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateTFCDecoder();
    auto factory = Module::TFC::CreateFactory();
    auto plugin = CreateStreamPlayerPlugin("TFC"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
