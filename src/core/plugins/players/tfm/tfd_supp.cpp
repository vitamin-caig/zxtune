/**
* 
* @file
*
* @brief  TurboFM Dump support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "tfm_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <formats/chiptune/fm/tfd.h>
#include <module/players/tfm/tfd.h>

namespace ZXTune
{
  void RegisterTFDSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'T', 'F', 'D', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateTFDDecoder();
    const Module::TFM::Factory::Ptr factory = Module::TFD::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
