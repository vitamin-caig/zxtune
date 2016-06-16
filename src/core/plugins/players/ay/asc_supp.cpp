/**
* 
* @file
*
* @brief  ASCSoundMaster support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <module/players/aym/ascsoundmaster.h>

namespace ZXTune
{
  void RegisterASCSupport(PlayerPluginsRegistrator& registrator)
  {
    {
      const Char ID[] = {'A', 'S', '0', 0};
      const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder = Formats::Chiptune::ASCSoundMaster::Ver0::CreateDecoder();
      const Module::AYM::Factory::Ptr factory = Module::ASCSoundMaster::CreateFactory(decoder);
      const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Char ID[] = {'A', 'S', 'C', 0};
      const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder = Formats::Chiptune::ASCSoundMaster::Ver1::CreateDecoder();
      const Module::AYM::Factory::Ptr factory = Module::ASCSoundMaster::CreateFactory(decoder);
      const PlayerPlugin::Ptr plugin = CreateTrackPlayerPlugin(ID, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
