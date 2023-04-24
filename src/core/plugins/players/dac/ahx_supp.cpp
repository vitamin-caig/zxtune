/**
 *
 * @file
 *
 * @brief  AHX support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/digital/abysshighestexperience.h>
#include <module/players/dac/abysshighestexperience.h>

namespace ZXTune
{
  void RegisterAHXSupport(PlayerPluginsRegistrator& registrator)
  {
    {
      const auto ID = "AHX"_id;
      const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;

      auto decoder = Formats::Chiptune::AbyssHighestExperience::CreateDecoder();
      auto factory = Module::AHX::CreateFactory(decoder);
      auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }

    {
      const auto ID = "HVL"_id;
      const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;

      auto decoder = Formats::Chiptune::AbyssHighestExperience::HivelyTracker::CreateDecoder();
      auto factory = Module::AHX::CreateFactory(decoder);
      auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
