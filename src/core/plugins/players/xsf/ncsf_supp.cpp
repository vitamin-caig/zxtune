/**
 *
 * @file
 *
 * @brief  NCSF support plugin
 *
 * @author liushuyu011@gmail.com
 *
 **/
// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <module/players/xsf/ncsf.h>

namespace ZXTune
{
  void RegisterNCSFSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const Char ID[] = {'N', 'C', 'S', 'F', 0};
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC
                        | Capabilities::Module::Traits::MULTIFILE;

    const auto factory = Module::NCSF::CreateFactory();
    const auto decoder = Formats::Chiptune::CreateNCSFDecoder();
    const auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune
