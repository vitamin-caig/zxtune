/**
* 
* @file
*
* @brief  YM/VTX support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/ay/aym_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <module/players/aym/ymvtx.h>

namespace ZXTune
{
  void RegisterVTXSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'V', 'T', 'X', 0};

    const Formats::Chiptune::YM::Decoder::Ptr decoder = Formats::Chiptune::YM::CreateVTXDecoder();
    const Module::AYM::Factory::Ptr factory = Module::YMVTX::CreateFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);;
    registrator.RegisterPlugin(plugin);
  }

  void RegisterYMSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'Y', 'M', 0};
    {
      const Formats::Chiptune::YM::Decoder::Ptr decoder = Formats::Chiptune::YM::CreatePackedYMDecoder();
      const Module::AYM::Factory::Ptr factory = Module::YMVTX::CreateFactory(decoder);
      const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Formats::Chiptune::YM::Decoder::Ptr decoder = Formats::Chiptune::YM::CreateYMDecoder();
      const Module::AYM::Factory::Ptr factory = Module::YMVTX::CreateFactory(decoder);
      const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);;
      registrator.RegisterPlugin(plugin);
    }
  }
}
