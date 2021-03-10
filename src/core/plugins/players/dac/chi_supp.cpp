/**
 *
 * @file
 *
 * @brief  ChipTracker support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/dac/dac_plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/chiptracker.h>
#include <module/players/dac/chiptracker.h>

namespace ZXTune
{
  void RegisterCHISupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const Char ID[] = {'C', 'H', 'I', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateChipTrackerDecoder();
    const Module::DAC::Factory::Ptr factory = Module::ChipTracker::CreateFactory();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune
