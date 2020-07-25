/**
* 
* @file
*
* @brief  ProTracker v2.x support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/ay/aym_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <formats/chiptune/aym/protracker2.h>
#include <module/players/aym/protracker2.h>

namespace ZXTune
{
  void RegisterPT2Support(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'T', '2', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProTracker2Decoder();
    const Module::AYM::Factory::Ptr factory = Module::ProTracker2::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);;
    registrator.RegisterPlugin(plugin);
  }
}
