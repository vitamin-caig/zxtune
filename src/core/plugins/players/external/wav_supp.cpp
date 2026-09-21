/**
 *
 * @file
 *
 * @brief  WAV support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "module/players/external/wav.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"

namespace ZXTune
{
  void RegisterWAVPlugin(PlayerPluginsRegistrator& registrator)
  {
    const auto ID = "WAV"_id;
    const uint_t CAPS = Capabilities::Module::Type::STREAM | Capabilities::Module::Device::DAC;

    auto decoder = Formats::Chiptune::CreateWAVDecoder();
    auto factory = Module::Wav::CreateFactory();
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
