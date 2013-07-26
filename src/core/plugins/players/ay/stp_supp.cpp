/*
Abstract:
  STP modules playback support

Last changed:
  $Id$

Author:
(C) Vitamin/CAIG/2001
*/

//local includes
#include "aym_plugin.h"
#include "soundtrackerpro.h"
#include "core/plugins/registrator.h"

namespace ZXTune
{
  void RegisterSTPSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'S', 'T', 'P', 0};

    const Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder = Formats::Chiptune::SoundTrackerPro::CreateCompiledModulesDecoder();
    const Module::AYM::Factory::Ptr factory = Module::SoundTrackerPro::CreateModulesFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
