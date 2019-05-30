/**
* 
* @file
*
* @brief  AY modules support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/ay/aym_conversion.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/emulation/ay.h>
#include <module/players/aym/ayemul.h>

namespace ZXTune
{
  void RegisterAYSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'A', 'Y', 0};
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::AY38910 | Capabilities::Module::Device::BEEPER
      | Module::AYM::GetSupportedFormatConvertors();

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateAYEMULDecoder();
    const Module::Factory::Ptr factory = Module::AYEMUL::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
