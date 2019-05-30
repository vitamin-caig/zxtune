/**
* 
* @file
*
* @brief  DigitalStudio support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/dac/dac_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/digitalstudio.h>
#include <module/players/dac/digitalstudio.h>

namespace ZXTune
{
  void RegisterDSTSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'D', 'S', 'T', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateDigitalStudioDecoder();
    const Module::DAC::Factory::Ptr factory = Module::DigitalStudio::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
