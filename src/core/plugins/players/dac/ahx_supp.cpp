/**
* 
* @file
*
* @brief  AHX support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/abysshighestexperience.h>
#include <module/players/dac/abysshighestexperience.h>

namespace ZXTune
{
  void RegisterAHXSupport(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'A', 'H', 'X', 0};
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;

    const Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr decoder = Formats::Chiptune::AbyssHighestExperience::CreateDecoder();
    const Module::Factory::Ptr factory = Module::AHX::CreateFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
