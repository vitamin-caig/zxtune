/**
 *
 * @file
 *
 * @brief  ProTracker v3.x support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_conversion.h"
#include "core/plugins/players/plugin.h"
// library includes
#include <module/players/aym/protracker3.h>

namespace ZXTune
{
  void RegisterPT3Support(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "PT3"_id;
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::AY38910
                        | Capabilities::Module::Device::TURBOSOUND | Module::AYM::GetSupportedFormatConvertors()
                        | Module::Vortex::GetSupportedFormatConvertors();

    auto decoder = Formats::Chiptune::ProTracker3::CreateDecoder();
    auto factory = Module::ProTracker3::CreateFactory(decoder);
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }

  void RegisterTXTSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "TXT"_id;
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::AY38910
                        | Module::AYM::GetSupportedFormatConvertors() | Module::Vortex::GetSupportedFormatConvertors();

    auto decoder = Formats::Chiptune::ProTracker3::VortexTracker2::CreateDecoder();
    auto factory = Module::ProTracker3::CreateFactory(decoder);
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
