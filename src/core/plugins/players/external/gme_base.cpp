/**
 *
 * @file
 *
 * @brief  Game Music Emu-based formats support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "core/plugins/players/plugin.h"
#include "module/players/external/gme.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"
#include "formats/multitrack/decoders.h"

#include "make_ptr.h"
#include "string_view.h"

namespace ZXTune::GME
{
  struct MultitrackPluginDescription
  {
    using MultitrackDecoderCreator = Formats::Multitrack::Decoder::Ptr (*)();
    using MultitrackFactoryCreator = Module::MultitrackFactory::Ptr (*)();

    const PluginId Id;
    const uint_t ChiptuneCaps;
    const MultitrackDecoderCreator CreateMultitrackDecoder;
    const MultitrackFactoryCreator CreateMultitrackFactory;
  };

  // clang-format off
  const MultitrackPluginDescription MULTITRACK_PLUGINS[] =
  {
    //nsf
    {
      "NSF"_id,
      Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::RP2A0X,
      &Formats::Multitrack::CreateNSFDecoder,
      &Module::GME::CreateNsfFactory,
    },
    //nsfe
    {
      "NSFE"_id,
      Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::RP2A0X,
      &Formats::Multitrack::CreateNSFEDecoder,
      &Module::GME::CreateNsfeFactory,
    },
    //gbs
    {
      "GBS"_id,
      Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::LR35902,
      &Formats::Multitrack::CreateGBSDecoder,
      &Module::GME::CreateGbsFactory,
    },
    //kssx
    {
      "KSSX"_id,
      Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::MULTI,
      &Formats::Multitrack::CreateKSSXDecoder,
      &Module::GME::CreateKssxFactory,
    },
    //hes
    {
      "HES"_id,
      Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::HUC6270,
      &Formats::Multitrack::CreateHESDecoder,
      &Module::GME::CreateHesFactory,
    },
  };
  // clang-format on

  struct SingletrackPluginDescription
  {
    using ChiptuneDecoderCreator = Formats::Chiptune::Decoder::Ptr (*)();
    using SingletrackFactoryCreator = Module::ExternalParsingFactory::Ptr (*)();

    const PluginId Id;
    const uint_t ChiptuneCaps;
    const ChiptuneDecoderCreator CreateChiptuneDecoder;
    const SingletrackFactoryCreator CreateSingletrackFactory;
  };

  // clang-format off
  const SingletrackPluginDescription SINGLETRACK_PLUGINS[] =
  {
    //gym
    {
      "GYM"_id,
      Capabilities::Module::Type::STREAM | Capabilities::Module::Device::MULTI,
      &Formats::Chiptune::CreateGYMDecoder,
      &Module::GME::CreateGymFactory,
    },
    //kss
    {
      "KSS"_id,
      Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::MULTI,
      &Formats::Chiptune::CreateKSSDecoder,
      &Module::GME::CreateKssFactory,
    },
  };
  // clang-format on
}

namespace ZXTune
{
  void RegisterGMEPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    for (const auto& desc : GME::MULTITRACK_PLUGINS)
    {
      auto decoder = desc.CreateMultitrackDecoder();
      auto factory = desc.CreateMultitrackFactory();
      {
        auto plugin = CreatePlayerPlugin(desc.Id, desc.ChiptuneCaps, decoder, factory);
        players.RegisterPlugin(std::move(plugin));
      }
      {
        auto factory = desc.CreateMultitrackFactory();
        auto plugin = CreateArchivePlugin(desc.Id, std::move(decoder), std::move(factory));
        archives.RegisterPlugin(std::move(plugin));
      }
    }
    for (const auto& desc : GME::SINGLETRACK_PLUGINS)
    {
      auto decoder = desc.CreateChiptuneDecoder();
      auto factory = desc.CreateSingletrackFactory();
      auto plugin = CreatePlayerPlugin(desc.Id, desc.ChiptuneCaps, std::move(decoder), std::move(factory));
      players.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
