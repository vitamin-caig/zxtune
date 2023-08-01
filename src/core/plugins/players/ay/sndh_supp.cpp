/**
 *
 * @file
 *
 * @brief  SNDH modules support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "module/players/aym/sndh.h"

#include "core/plugin_attrs.h"
#include "formats/multitrack/decoders.h"

#include <utility>

namespace ZXTune
{
  void RegisterSNDHSupport(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    const auto ID = "SNDH"_id;
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::AY38910
                        | Capabilities::Module::Conversion::PSG | Capabilities::Module::Conversion::ZX50
                        | Capabilities::Module::Conversion::AYDUMP | Capabilities::Module::Conversion::FYM;
    auto decoder = Formats::Multitrack::CreateSNDHDecoder();
    auto factory = Module::SNDH::CreateFactory();
    {
      auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
      players.RegisterPlugin(std::move(plugin));
    }
    {
      auto plugin = CreateArchivePlugin(ID, std::move(decoder), std::move(factory));
      archives.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
