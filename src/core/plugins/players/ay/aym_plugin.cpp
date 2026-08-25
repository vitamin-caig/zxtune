/**
 *
 * @file
 *
 * @brief  AYM-based player plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/ay/aym_plugin.h"

#include "core/plugins/players/plugin.h"

#include "core/plugin_attrs.h"

#include <utility>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::AYM::Factory::Ptr factory)
  {
    auto modFactory = Module::AYM::CreateModuleFactory(std::move(factory));
    const uint_t ayCaps = Capabilities::Module::Device::AY38910 | Capabilities::Module::Conversion::PSG
                          | Capabilities::Module::Conversion::ZX50 | Capabilities::Module::Conversion::AYDUMP
                          | Capabilities::Module::Conversion::FYM;
    return CreatePlayerPlugin(id, caps | ayCaps, std::move(decoder), std::move(modFactory));
  }

  PlayerPlugin::Ptr CreateTrackPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                            Module::AYM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::TRACK, std::move(decoder), std::move(factory));
  }

  PlayerPlugin::Ptr CreateStreamPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                             Module::AYM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::STREAM, std::move(decoder), std::move(factory));
  }
}  // namespace ZXTune
