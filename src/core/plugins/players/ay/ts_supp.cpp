/**
 *
 * @file
 *
 * @brief  TurboSound containers support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multi/plugins_factory.h"
#include "core/plugins/players/plugin.h"
#include "module/players/aym/ts.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"

namespace ZXTune
{
  void RegisterTSSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "TS"_id;
    const uint_t CAPS = Capabilities::Module::Type::MULTI | Capabilities::Module::Device::TURBOSOUND;

    auto decoder = Formats::Chiptune::CreateTurboSoundDecoder();
    auto factory = Module::TS::CreateFactory(CreatePluginsDelegateFactory());
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
