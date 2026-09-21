/**
 *
 * @file
 *
 * @brief  SID support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "core/plugins/players/plugin.h"
#include "module/players/external/sid.h"

#include "core/plugin_attrs.h"
#include "formats/multitrack/decoders.h"

namespace ZXTune
{
  void RegisterSIDPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    const auto ID = "SID"_id;
    auto decoder = Formats::Multitrack::CreateSIDDecoder();
    auto factory = Module::Sid::CreateFactory();
    {
      const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::MOS6581;
      auto plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
      players.RegisterPlugin(std::move(plugin));
    }
    {
      auto plugin = CreateArchivePlugin(ID, std::move(decoder), std::move(factory));
      archives.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
