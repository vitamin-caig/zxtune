/**
 *
 * @file
 *
 * @brief  Uncompiled SoundTracker modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
// library includes
#include <formats/chiptune/aym/soundtracker.h>
#include <module/players/aym/soundtracker.h>

namespace ZXTune
{
  void RegisterST1Support(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const Char ID[] = {'S', 'T', '1', 0};

    const Formats::Chiptune::SoundTracker::Decoder::Ptr decoder =
        Formats::Chiptune::SoundTracker::Ver1::CreateUncompiledDecoder();
    const Module::AYM::Factory::Ptr factory = Module::SoundTracker::CreateFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune
