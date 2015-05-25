/**
* 
* @file
*
* @brief  SoundTracker v3 modules support
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
  void RegisterST3Support(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'S', 'T', '3', 0};

    const Formats::Chiptune::SoundTracker::Decoder::Ptr decoder = Formats::Chiptune::SoundTracker::Ver3::CreateDecoder();
    const Module::AYM::Factory::Ptr factory = Module::SoundTracker::CreateModulesFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
