/**
 *
 * @file
 *
 * @brief  DAC-based player plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/dac/dac_plugin.h"

#include "core/plugins/players/plugin.h"

#include "core/plugin_attrs.h"

#include <utility>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::DAC::Factory::Ptr factory)
  {
    auto modFactory = Module::DAC::CreateModuleFactory(std::move(factory));
    const uint_t caps = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;
    return CreatePlayerPlugin(id, caps, std::move(decoder), std::move(modFactory));
  }
}  // namespace ZXTune
