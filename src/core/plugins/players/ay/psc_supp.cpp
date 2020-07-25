/**
* 
* @file
*
* @brief  ProSoundCreator support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/ay/aym_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <formats/chiptune/aym/prosoundcreator.h>
#include <module/players/aym/prosoundcreator.h>

namespace ZXTune
{
  void RegisterPSCSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'S', 'C', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProSoundCreatorDecoder();
    const Module::AYM::Factory::Ptr factory = Module::ProSoundCreator::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);;
    registrator.RegisterPlugin(plugin);
  }
}
