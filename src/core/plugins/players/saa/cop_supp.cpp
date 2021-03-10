/**
 *
 * @file
 *
 * @brief  ETracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/saa/etracker.h>
#include <module/players/saa/etracker.h>

namespace ZXTune
{
  void RegisterCOPSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const Char ID[] = {'C', 'O', 'P', 0};
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::SAA1099;

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateETrackerDecoder();
    const Module::Factory::Ptr factory = Module::ETracker::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune
