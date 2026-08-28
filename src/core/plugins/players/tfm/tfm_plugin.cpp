/**
 *
 * @file
 *
 * @brief  TFM-based player plugins factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "module/players/tfm/all.h"
#include "module/players/tfm/tfm_factory.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"

#include <utility>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::TFM::Factory create)
  {
    auto modFactory = Module::TFM::CreateModuleFactory(create);
    const uint_t tfmCaps = Capabilities::Module::Device::TURBOFM;
    return CreatePlayerPlugin(id, caps | tfmCaps, std::move(decoder), std::move(modFactory));
  }

  PlayerPlugin::Ptr CreateTrackPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                            Module::TFM::Factory create)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::TRACK, std::move(decoder), create);
  }

  PlayerPlugin::Ptr CreateStreamPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                             Module::TFM::Factory create)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::STREAM, std::move(decoder), create);
  }

  void RegisterTFMPlugins(PlayerPluginsRegistrator& registrator)
  {
    using namespace Formats::Chiptune;
    using namespace Module::TFM;
    {
      auto decoder = CreateTFDDecoder();
      auto plugin = CreateStreamPlayerPlugin("TFD"_id, std::move(decoder), &CreateTFDChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateTFCDecoder();
      auto plugin = CreateStreamPlayerPlugin("TFC"_id, std::move(decoder), &CreateTFCChiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateTFMMusicMaker05Decoder();
      auto plugin = CreateTrackPlayerPlugin("TF0"_id, std::move(decoder), &CreateTFMMusicMaker05Chiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateTFMMusicMaker13Decoder();
      auto plugin = CreateTrackPlayerPlugin("TFE"_id, std::move(decoder), &CreateTFMMusicMaker13Chiptune);
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
