/**
* 
* @file
*
* @brief  Compiled SoundTracker modules support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/ay/aym_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <formats/chiptune/aym/soundtracker.h>
#include <module/players/aym/soundtracker.h>

namespace ZXTune
{
  void RegisterSTCSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'S', 'T', 'C', 0};

    const Formats::Chiptune::SoundTracker::Decoder::Ptr decoder = Formats::Chiptune::SoundTracker::Ver1::CreateCompiledDecoder();
    const Module::AYM::Factory::Ptr factory = Module::SoundTracker::CreateFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
