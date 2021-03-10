/**
 *
 * @file
 *
 * @brief  SampleTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/
// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/sampletracker.h>
#include <module/players/dac/sampletracker.h>

namespace ZXTune
{
  void RegisterSTRSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const Char ID[] = {'S', 'T', 'R', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSampleTrackerDecoder();
    const Module::DAC::Factory::Ptr factory = Module::SampleTracker::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune
