/**
* 
* @file
*
* @brief  ProTracker v1.x support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <formats/chiptune/aym/protracker1.h>
#include <module/players/aym/protracker1.h>

namespace ZXTune
{
  void RegisterPT1Support(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'T', '1', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProTracker1Decoder();
    const Module::AYM::Factory::Ptr factory = Module::ProTracker1::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);;
    registrator.RegisterPlugin(plugin);
  }
}
