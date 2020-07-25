/**
* 
* @file
*
* @brief  SSF/DSF support plugin
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
#include <module/players/xsf/sdsf.h>

namespace ZXTune
{
  void RegisterSDSFSupport(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC | Capabilities::Module::Traits::MULTIFILE;
    const auto factory = Module::SDSF::CreateFactory();
    {
      //plugin attributes
      const Char ID[] = {'S', 'S', 'F', 0};

      const auto decoder = Formats::Chiptune::CreateSSFDecoder();
      const auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      //plugin attributes
      const Char ID[] = {'D', 'S', 'F', 0};

      const auto decoder = Formats::Chiptune::CreateDSFDecoder();
      const auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
