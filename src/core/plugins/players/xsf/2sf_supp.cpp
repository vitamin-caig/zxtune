/**
* 
* @file
*
* @brief  2SF support plugin
*
* @author vitamin.caig@gmail.com
*
**/
//local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <module/players/xsf/2sf.h>

namespace ZXTune
{
  void Register2SFSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'2', 'S', 'F', 0};
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC | Capabilities::Module::Traits::MULTIFILE;

    const auto factory = Module::TwoSF::CreateFactory();
    const auto decoder = Formats::Chiptune::Create2SFDecoder();
    const auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
