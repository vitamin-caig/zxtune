/**
 *
 * @file
 *
 * @brief  DigitalMusicMaker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/digitalmusicmaker.h>
#include <module/players/dac/digitalmusicmaker.h>

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
