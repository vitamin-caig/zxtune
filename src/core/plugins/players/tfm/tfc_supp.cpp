/**
* 
* @file
*
* @brief  TurboFM Compiled support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/tfm/tfm_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/fm/tfc.h>
#include <module/players/tfm/tfc.h>

namespace ZXTune
{
  void RegisterTFCSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'T', 'F', 'C', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateTFCDecoder();
    const Module::TFM::Factory::Ptr factory = Module::TFC::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
