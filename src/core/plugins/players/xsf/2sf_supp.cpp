/**
 *
 * @file
 *
 * @brief  2SF support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/decoders.h"
#include "module/players/xsf/2sf.h"
#include "module/players/xsf/xsf_factory.h"

#include "core/plugin_attrs.h"

#include "types.h"

#include <utility>

namespace ZXTune
{
  void Register2SFSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "2SF"_id;
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC
                        | Capabilities::Module::Traits::MULTIFILE;

    auto factory = Module::TwoSF::CreateFactory();
    auto decoder = Formats::Chiptune::Create2SFDecoder();
    auto plugin =
        CreatePlayerPlugin(ID, CAPS, std::move(decoder), Module::XSF::CreateModuleFactory(std::move(factory)));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
