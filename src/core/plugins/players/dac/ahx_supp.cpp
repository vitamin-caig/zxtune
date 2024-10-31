/**
 *
 * @file
 *
 * @brief  AHX support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/digital/abysshighestexperience.h"
#include "module/players/dac/abysshighestexperience.h"

#include "core/plugin_attrs.h"

#include "types.h"

#include <utility>

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
