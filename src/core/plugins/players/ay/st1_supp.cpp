/*
Abstract:
  ST1 modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

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
