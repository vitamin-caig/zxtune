/**
* 
* @file
*
* @brief  Uncompiled SoundTracker modules support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_plugin.h"
#include "soundtracker.h"
#include "core/plugins/registrator.h"

namespace ZXTune
{
  void RegisterST1Support(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'S', 'T', '1', 0};

    const Formats::Chiptune::SoundTracker::Decoder::Ptr decoder = Formats::Chiptune::SoundTracker::Ver1::CreateUncompiledDecoder();
    const Module::AYM::Factory::Ptr factory = Module::SoundTracker::CreateModulesFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
