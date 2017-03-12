/**
* 
* @file
*
* @brief  TFMMusicMaker support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "tfm_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <module/players/tfm/tfmmusicmaker.h>

namespace ZXTune
{
  void RegisterTFESupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    {
      const Char ID[] = {'T', 'F', '0', 0};
      const Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder = Formats::Chiptune::TFMMusicMaker::Ver05::CreateDecoder();
      const Module::TFM::Factory::Ptr factory = Module::TFMMusicMaker::CreateFactory(decoder);
      const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Char ID[] = {'T', 'F', 'E', 0};
      const Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder = Formats::Chiptune::TFMMusicMaker::Ver13::CreateDecoder();
      const Module::TFM::Factory::Ptr factory = Module::TFMMusicMaker::CreateFactory(decoder);
      const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
