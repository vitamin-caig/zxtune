/**
 *
 * @file
 *
 * @brief  V2M support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include <formats/chiptune/decoders.h>
#include <module/players/dac/v2m.h>

#include <core/plugin_attrs.h>

namespace ZXTune
{
  void RegisterV2MSupport(PlayerPluginsRegistrator& registrator)
  {
    const auto ID = "V2M"_id;
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;

    auto decoder = Formats::Chiptune::CreateV2MDecoder();
    auto factory = Module::V2M::CreateFactory();
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
