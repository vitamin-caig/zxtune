/**
 *
 * @file
 *
 * @brief  ASAP-based formats support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "core/plugins/players/plugin.h"
#include "module/players/external/asap.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"
#include "formats/multitrack/decoders.h"

#include "make_ptr.h"
#include "string_view.h"

namespace ZXTune::ASAP
{
  struct MultitrackPluginDescription
  {
    using MultitrackDecoderCreator = Formats::Multitrack::Decoder::Ptr (*)();

    const PluginId Id;
    const uint_t ChiptuneCaps;
    const MultitrackDecoderCreator CreateMultitrackDecoder;
  };

  // clang-format off
  const MultitrackPluginDescription MULTITRACK_PLUGINS[] =
  {
    //sap
    {
      "SAP"_id,
      Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::CO12294,
      &Formats::Multitrack::CreateSAPDecoder,
    }
  };
  // clang-format on

  struct SingletrackPluginDescription
  {
    using ChiptuneDecoderCreator = Formats::Chiptune::Decoder::Ptr (*)();

    const PluginId Id;
    const uint_t ChiptuneCaps;
    const ChiptuneDecoderCreator CreateChiptuneDecoder;
  };

  // clang-format off
  const SingletrackPluginDescription SINGLETRACK_PLUGINS[] =
  {
    //rmt
    {
      "RMT"_id,
      Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::CO12294,
      &Formats::Chiptune::CreateRasterMusicTrackerDecoder,
    },
  };
  // clang-format on
}

namespace ZXTune
{
  void RegisterASAPPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    for (const auto& desc : ASAP::MULTITRACK_PLUGINS)
    {
      auto decoder = desc.CreateMultitrackDecoder();
      auto factory = Module::ASAP::CreateMultitrackFactory(desc.Id);
      {
        auto plugin = CreatePlayerPlugin(desc.Id, desc.ChiptuneCaps, decoder, factory);
        players.RegisterPlugin(std::move(plugin));
      }
      {
        auto plugin = CreateArchivePlugin(desc.Id, std::move(decoder), std::move(factory));
        archives.RegisterPlugin(std::move(plugin));
      }
    }
    for (const auto& desc : ASAP::SINGLETRACK_PLUGINS)
    {
      auto decoder = desc.CreateChiptuneDecoder();
      auto factory = Module::ASAP::CreateFactory(desc.Id);
      auto plugin = CreatePlayerPlugin(desc.Id, desc.ChiptuneCaps, std::move(decoder), std::move(factory));
      players.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
