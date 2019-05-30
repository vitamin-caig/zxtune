/**
* 
* @file
*
* @brief  DigitalMusicMaker support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/dac/dac_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/digitalmusicmaker.h>
#include <module/players/dac/digitalmusicmaker.h>

namespace ZXTune
{
  void RegisterDMMSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'D', 'M', 'M', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateDigitalMusicMakerDecoder();
    const Module::DAC::Factory::Ptr factory = Module::DigitalMusicMaker::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
