/**
 *
 * @file
 *
 * @brief  AYC support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
// library includes
#include <formats/chiptune/aym/ayc.h>
#include <module/players/aym/ayc.h>

namespace ZXTune
{
  void RegisterAYCSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const Char ID[] = {'A', 'Y', 'C', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateAYCDecoder();
    const Module::AYM::Factory::Ptr factory = Module::AYC::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune
