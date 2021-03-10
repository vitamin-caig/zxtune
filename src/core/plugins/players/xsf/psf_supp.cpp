/**
 *
 * @file
 *
 * @brief  PSF1 support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/
// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <module/players/xsf/psf.h>

namespace ZXTune
{
  void RegisterPSFSupport(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC
                        | Capabilities::Module::Traits::MULTIFILE;
    const auto factory = Module::PSF::CreateFactory();
    {
      // plugin attributes
      const Char ID[] = {'P', 'S', 'F', 0};

      const auto decoder = Formats::Chiptune::CreatePSFDecoder();
      const auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      // plugin attributes
      const Char ID[] = {'P', 'S', 'F', '2', 0};

      const auto decoder = Formats::Chiptune::CreatePSF2Decoder();
      const auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}  // namespace ZXTune
