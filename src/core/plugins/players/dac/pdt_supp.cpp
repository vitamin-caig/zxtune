/**
* 
* @file
*
* @brief  ProDigiTracker support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/dac/dac_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/prodigitracker.h>
#include <module/players/dac/prodigitracker.h>

namespace ZXTune
{
  void RegisterPDTSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'D', 'T', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProDigiTrackerDecoder();
    const Module::DAC::Factory::Ptr factory = Module::ProDigiTracker::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
