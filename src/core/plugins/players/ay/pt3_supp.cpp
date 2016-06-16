/**
* 
* @file
*
* @brief  ProTracker v3.x support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_conversion.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <module/players/aym/protracker3.h>

namespace ZXTune
{
  void RegisterPT3Support(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'T', '3', 0};
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::AY38910 | Capabilities::Module::Device::TURBOSOUND
      | Module::AYM::GetSupportedFormatConvertors() | Module::Vortex::GetSupportedFormatConvertors();

    const Formats::Chiptune::ProTracker3::Decoder::Ptr decoder = Formats::Chiptune::ProTracker3::CreateDecoder();
    const Module::Factory::Ptr factory = Module::ProTracker3::CreateFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }

  void RegisterTXTSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'T', 'X', 'T', 0};
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::AY38910
      | Module::AYM::GetSupportedFormatConvertors() | Module::Vortex::GetSupportedFormatConvertors();

    const Formats::Chiptune::ProTracker3::Decoder::Ptr decoder = Formats::Chiptune::ProTracker3::VortexTracker2::CreateDecoder();
    const Module::Factory::Ptr factory = Module::ProTracker3::CreateFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
