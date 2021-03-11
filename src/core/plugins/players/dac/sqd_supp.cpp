/**
 *
 * @file
 *
 * @brief  SQDigitalTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/sqdigitaltracker.h>
#include <module/players/dac/sqdigitaltracker.h>

namespace ZXTune
{
  void RegisterSQDSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const Char ID[] = {'S', 'Q', 'D', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSQDigitalTrackerDecoder();
    const Module::DAC::Factory::Ptr factory = Module::SQDigitalTracker::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune
