/**
 *
 * @file
 *
 * @brief  PSF1 support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/decoders.h"
#include "module/players/xsf/psf.h"
#include "module/players/xsf/xsf_factory.h"

#include "core/plugin_attrs.h"

#include "types.h"

#include <utility>

namespace ZXTune
{
  void RegisterPSFSupport(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC
                        | Capabilities::Module::Traits::MULTIFILE;
    auto factory = Module::PSF::CreateFactory();
    {
      auto decoder = Formats::Chiptune::CreatePSFDecoder();
      auto plugin = CreatePlayerPlugin("PSF"_id, CAPS, std::move(decoder), Module::XSF::CreateModuleFactory(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = Formats::Chiptune::CreatePSF2Decoder();
      auto plugin =
          CreatePlayerPlugin("PSF2"_id, CAPS, std::move(decoder), Module::XSF::CreateModuleFactory(std::move(factory)));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
