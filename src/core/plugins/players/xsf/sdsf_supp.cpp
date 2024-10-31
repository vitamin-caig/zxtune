/**
 *
 * @file
 *
 * @brief  SSF/DSF support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include <formats/chiptune/decoders.h>
#include <module/players/xsf/sdsf.h>

#include <core/plugin_attrs.h>

namespace ZXTune
{
  void RegisterSDSFSupport(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC
                        | Capabilities::Module::Traits::MULTIFILE;
    auto factory = Module::SDSF::CreateFactory();
    {
      auto decoder = Formats::Chiptune::CreateSSFDecoder();
      auto plugin = CreatePlayerPlugin("SSF"_id, CAPS, std::move(decoder), Module::XSF::CreateModuleFactory(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = Formats::Chiptune::CreateDSFDecoder();
      auto plugin =
          CreatePlayerPlugin("DSF"_id, CAPS, std::move(decoder), Module::XSF::CreateModuleFactory(std::move(factory)));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
