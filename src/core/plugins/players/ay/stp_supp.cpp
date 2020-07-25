/**
* 
* @file
*
* @brief  Compiled SoundTrackerPro modules support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/ay/aym_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <formats/chiptune/aym/soundtrackerpro.h>
#include <module/players/aym/soundtrackerpro.h>

namespace ZXTune
{
  void RegisterSTPSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'S', 'T', 'P', 0};

    const Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder = Formats::Chiptune::SoundTrackerPro::CreateCompiledModulesDecoder();
    const Module::AYM::Factory::Ptr factory = Module::SoundTrackerPro::CreateFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
