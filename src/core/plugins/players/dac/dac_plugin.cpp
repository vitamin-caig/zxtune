/**
 *
 * @file
 *
 * @brief  DAC-based player plugins factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "module/players/dac/abysshighestexperience.h"
#include "module/players/dac/all.h"
#include "module/players/dac/dac_factory.h"
#include "module/players/dac/v2m.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"

#include <utility>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::Factory::Ptr factory)
  {
    const uint_t caps = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;
    return CreatePlayerPlugin(id, caps, std::move(decoder), std::move(factory));
  }

  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::DAC::Factory create)
  {
    auto factory = Module::DAC::CreateModuleFactory(create);
    return CreatePlayerPlugin(id, std::move(decoder), std::move(factory));
  }

  void RegisterDACPlugins(PlayerPluginsRegistrator& registrator)
  {
    using namespace Formats::Chiptune;
    using namespace Module::DAC;
    {
      auto decoder = CreateProDigiTrackerDecoder();
      auto plugin = CreatePlayerPlugin("PDT"_id, std::move(decoder), &CreateProDigiTrackerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateChipTrackerDecoder();
      auto plugin = CreatePlayerPlugin("CHI"_id, std::move(decoder), &CreateChipTrackerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateSampleTrackerDecoder();
      auto plugin = CreatePlayerPlugin("STR"_id, std::move(decoder), &CreateSampleTrackerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateDigitalStudioDecoder();
      auto plugin = CreatePlayerPlugin("DST"_id, std::move(decoder), &CreateDigitalStudioChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateSQDigitalTrackerDecoder();
      auto plugin = CreatePlayerPlugin("SQD"_id, std::move(decoder), &CreateSQDigitalTrackerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateDigitalMusicMakerDecoder();
      auto plugin = CreatePlayerPlugin("DMM"_id, std::move(decoder), &CreateDigitalMusicMakerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateExtremeTracker1Decoder();
      auto plugin = CreatePlayerPlugin("ET1"_id, std::move(decoder), &CreateExtremeTracker1Chiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateAbyssHighestExperienceDecoder();
      auto factory = Module::AHX::CreateFactory(AbyssHighestExperience::Parse);
      auto plugin = CreatePlayerPlugin("AHX"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateHivelyTrackerDecoder();
      auto factory = Module::AHX::CreateFactory(AbyssHighestExperience::HivelyTracker::Parse);
      auto plugin = CreatePlayerPlugin("HVL"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateV2MDecoder();
      auto factory = Module::V2M::CreateFactory();
      auto plugin = CreatePlayerPlugin("V2M"_id, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
