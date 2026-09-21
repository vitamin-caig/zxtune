/**
 *
 * @file
 *
 * @brief  MTC support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multi/plugins_factory.h"
#include "core/plugins/players/plugin.h"
#include "module/players/multitrack/mtc.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"

namespace ZXTune
{
  void RegisterMTCSupport(PlayerPluginsRegistrator& registrator)
  {
    const auto ID = "MTC"_id;
    const uint_t CAPS = Capabilities::Module::Type::MULTI | Capabilities::Module::Device::MULTI;

    auto decoder = Formats::Chiptune::CreateMultiTrackContainerDecoder();
    auto factory = Module::MTC::CreateFactory(CreatePluginsDelegateFactory());
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
