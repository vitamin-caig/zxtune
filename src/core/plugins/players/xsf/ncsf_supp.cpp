/**
 *
 * @file
 *
 * @brief  NCSF support plugin
 *
 * @author liushuyu011@gmail.com
 *
 **/
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/decoders.h"
#include "module/players/xsf/ncsf.h"
#include "module/players/xsf/xsf_factory.h"

#include "core/plugin_attrs.h"

#include "types.h"

#include <utility>

namespace ZXTune
{
  void RegisterNCSFSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "NCSF"_id;
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC
                        | Capabilities::Module::Traits::MULTIFILE;

    auto factory = Module::NCSF::CreateFactory();
    auto decoder = Formats::Chiptune::CreateNCSFDecoder();
    auto plugin =
        CreatePlayerPlugin(ID, CAPS, std::move(decoder), Module::XSF::CreateModuleFactory(std::move(factory)));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
