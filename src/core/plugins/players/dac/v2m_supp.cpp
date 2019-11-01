/**
* 
* @file
*
* @brief  V2M support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <module/players/dac/v2m.h>

namespace ZXTune
{
  void RegisterV2MSupport(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'V', '2', 'M', 0};
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;

    const auto decoder = Formats::Chiptune::CreateV2MDecoder();
    const auto factory = Module::V2M::CreateFactory();
    const auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
