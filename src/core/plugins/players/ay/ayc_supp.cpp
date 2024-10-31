/**
 *
 * @file
 *
 * @brief  AYC support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include "formats/chiptune/aym/ayc.h"
#include "module/players/aym/ayc.h"

namespace ZXTune
{
  void RegisterAYCSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateAYCDecoder();
    auto factory = Module::AYC::CreateFactory();
    auto plugin = CreateStreamPlayerPlugin("AYC"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
