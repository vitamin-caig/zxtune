/**
 *
 * @file
 *
 * @brief  ProSoundMaker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
// library includes
#include <formats/chiptune/aym/prosoundmaker.h>
#include <module/players/aym/prosoundmaker.h>

namespace ZXTune
{
  void RegisterPSMSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const Char ID[] = {'P', 'S', 'M', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProSoundMakerCompiledDecoder();
    const Module::AYM::Factory::Ptr factory = Module::ProSoundMaker::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);
    ;
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune
