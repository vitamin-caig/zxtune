/**
 *
 * @file
 *
 * @brief  USF support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include <formats/chiptune/decoders.h>
#include <module/players/xsf/usf.h>

#include <core/plugin_attrs.h>

namespace ZXTune
{
  void RegisterUSFSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "USF"_id;
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC
                        | Capabilities::Module::Traits::MULTIFILE;

    auto factory = Module::USF::CreateFactory();
    auto decoder = Formats::Chiptune::CreateUSFDecoder();
    auto plugin =
        CreatePlayerPlugin(ID, CAPS, std::move(decoder), Module::XSF::CreateModuleFactory(std::move(factory)));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
