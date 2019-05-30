/**
* 
* @file
*
* @brief  ExtremeTracker1 support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/dac/dac_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/extremetracker1.h>
#include <module/players/dac/extremetracker1.h>

namespace ZXTune
{
  void RegisterET1Support(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'E', 'T', '1', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateExtremeTracker1Decoder();
    const Module::DAC::Factory::Ptr factory = Module::ExtremeTracker1::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
