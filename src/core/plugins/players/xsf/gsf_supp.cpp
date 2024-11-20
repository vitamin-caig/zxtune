/**
 *
 * @file
 *
 * @brief  GSF support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/decoders.h"
#include "module/players/xsf/gsf.h"

#include "core/plugin_attrs.h"

namespace ZXTune
{
  void RegisterGSFSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "GSF"_id;
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC
                        | Capabilities::Module::Traits::MULTIFILE;

    auto factory = Module::GSF::CreateFactory();
    auto decoder = Formats::Chiptune::CreateGSFDecoder();
    auto plugin =
        CreatePlayerPlugin(ID, CAPS, std::move(decoder), Module::XSF::CreateModuleFactory(std::move(factory)));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
