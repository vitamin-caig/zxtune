/**
 *
 * @file
 *
 * @brief  DigitalStudio support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
#include "formats/chiptune/digital/digitalstudio.h"
#include "module/players/dac/digitalstudio.h"

#include "core/plugin_attrs.h"

#include <utility>

namespace ZXTune
{
  void RegisterDSTSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateDigitalStudioDecoder();
    auto factory = Module::DigitalStudio::CreateFactory();
    auto plugin = CreatePlayerPlugin("DST"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
