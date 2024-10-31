/**
 *
 * @file
 *
 * @brief  DigitalMusicMaker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
#include "formats/chiptune/digital/digitalmusicmaker.h"
#include "module/players/dac/digitalmusicmaker.h"

#include "core/plugin_attrs.h"

#include <utility>

namespace ZXTune
{
  void RegisterDMMSupport(PlayerPluginsRegistrator& registrator)
  {
    auto decoder = Formats::Chiptune::CreateDigitalMusicMakerDecoder();
    auto factory = Module::DigitalMusicMaker::CreateFactory();
    auto plugin = CreatePlayerPlugin("DMM"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
