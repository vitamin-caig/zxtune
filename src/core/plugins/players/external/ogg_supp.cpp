/**
 *
 * @file
 *
 * @brief  OGG support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "module/players/external/ogg.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"

namespace ZXTune
{
  void RegisterOGGPlugin(PlayerPluginsRegistrator& registrator)
  {
    const auto ID = "OGG"_id;
    const uint_t CAPS = Capabilities::Module::Type::STREAM | Capabilities::Module::Device::DAC;

    auto decoder = Formats::Chiptune::CreateOGGDecoder();
    auto factory = Module::Ogg::CreateFactory();
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
