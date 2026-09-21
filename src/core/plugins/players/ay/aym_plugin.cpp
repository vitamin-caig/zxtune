/**
 *
 * @file
 *
 * @brief  AYM-based player plugins factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multi/plugins_factory.h"
#include "core/plugins/players/plugin.h"
#include "module/players/aym/all.h"
#include "module/players/aym/ayemul.h"
#include "module/players/aym/aym_factory.h"
#include "module/players/aym/protracker3.h"
#include "module/players/aym/ts.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"

#include <utility>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::AYM::ChiptuneCreator create)
  {
    auto factory = Module::AYM::CreateModuleFactory(create);
    const uint_t ayCaps = Capabilities::Module::Device::AY38910 | Capabilities::Module::Conversion::PSG
                          | Capabilities::Module::Conversion::ZX50 | Capabilities::Module::Conversion::AYDUMP
                          | Capabilities::Module::Conversion::FYM;
    return CreatePlayerPlugin(id, caps | ayCaps, std::move(decoder), std::move(factory));
  }

  PlayerPlugin::Ptr CreateTrackPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                            Module::AYM::ChiptuneCreator create)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::TRACK, std::move(decoder), create);
  }

  PlayerPlugin::Ptr CreateStreamPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                             Module::AYM::ChiptuneCreator create)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::STREAM, std::move(decoder), create);
  }

  void RegisterAYMPlugins(PlayerPluginsRegistrator& registrator)
  {
    using namespace Formats::Chiptune;
    using namespace Module::AYM;
    // Register TS first
    {
      const uint_t CAPS = Capabilities::Module::Type::MULTI | Capabilities::Module::Device::TURBOSOUND;

      auto decoder = CreateTurboSoundDecoder();
      auto factory = Module::TS::CreateFactory(CreatePluginsDelegateFactory());
      auto plugin = CreatePlayerPlugin("TS"_id, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::AY38910
                          | Capabilities::Module::Device::BEEPER;

      auto decoder = CreateAYEMULDecoder();
      auto factory = Module::AYEMUL::CreateFactory();
      auto plugin = CreatePlayerPlugin("AY"_id, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::AY38910
                          | Capabilities::Module::Device::TURBOSOUND;

      auto decoder = CreateProTracker3Decoder();
      auto factory = Module::ProTracker3::CreateFactory(ProTracker3::Parse);
      auto plugin = CreatePlayerPlugin("PT3"_id, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::AY38910;

      auto decoder = CreateVortexTracker2Decoder();
      auto factory = Module::ProTracker3::CreateFactory(ProTracker3::VortexTracker2::Parse);
      auto plugin = CreatePlayerPlugin("TXT"_id, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateProTracker2Decoder();
      auto plugin = CreateTrackPlayerPlugin("PT2"_id, std::move(decoder), &CreateProTracker2Chiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateSoundTrackerCompiledDecoder();
      auto plugin = CreateTrackPlayerPlugin("STC"_id, std::move(decoder), &CreateSoundTrackerCompiledChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateSoundTrackerDecoder();
      auto plugin = CreateTrackPlayerPlugin("ST1"_id, std::move(decoder), &CreateSoundTrackerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateSoundTracker3Decoder();
      auto plugin = CreateTrackPlayerPlugin("ST3"_id, std::move(decoder), &CreateSoundTracker3Chiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateASCSoundMaster0xDecoder();
      auto plugin = CreateTrackPlayerPlugin("AS0"_id, std::move(decoder), &CreateASCSoundMaster0Chiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateASCSoundMaster1xDecoder();
      auto plugin = CreateTrackPlayerPlugin("ASC"_id, std::move(decoder), &CreateASCSoundMasterChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateSoundTrackerProCompiledDecoder();
      auto plugin = CreateTrackPlayerPlugin("STP"_id, std::move(decoder), &CreateSoundTrackerProChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreatePSGDecoder();
      auto plugin = CreateStreamPlayerPlugin("PSG"_id, std::move(decoder), &CreatePSGChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateProSoundMakerCompiledDecoder();
      auto plugin = CreateTrackPlayerPlugin("PSM"_id, std::move(decoder), &CreateProSoundMakerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateGlobalTrackerDecoder();
      auto plugin = CreateTrackPlayerPlugin("GTR"_id, std::move(decoder), &CreateGlobalTrackerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateProTracker1Decoder();
      auto plugin = CreateTrackPlayerPlugin("PT1"_id, std::move(decoder), &CreateProTracker1Chiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateVTXDecoder();
      auto plugin = CreateStreamPlayerPlugin("VTX"_id, std::move(decoder), &CreateVTXChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreatePackedYMDecoder();
      auto plugin = CreateStreamPlayerPlugin("YM"_id, std::move(decoder), &CreateYMPackedChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateYMDecoder();
      auto plugin = CreateStreamPlayerPlugin("YM"_id, std::move(decoder), &CreateYMChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateSQTrackerDecoder();
      auto plugin = CreateTrackPlayerPlugin("SQT"_id, std::move(decoder), &CreateSQTrackerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateProSoundCreatorDecoder();
      auto plugin = CreateTrackPlayerPlugin("PSC"_id, std::move(decoder), &CreateProSoundCreatorChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateFastTrackerDecoder();
      auto plugin = CreateTrackPlayerPlugin("FTC"_id, std::move(decoder), &CreateFastTrackerChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateAYCDecoder();
      auto plugin = CreateStreamPlayerPlugin("AYC"_id, std::move(decoder), &CreateAYCChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
