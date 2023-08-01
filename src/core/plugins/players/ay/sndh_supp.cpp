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
#include "core/plugins/players/ay/aym_conversion.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "formats/multitrack/decoders.h"
#include "formats/multitrack/sndh.h"
#include "module/players/aym/sndh.h"

#include "core/plugin_attrs.h"

namespace ZXTune
{
  void RegisterSNDHSupport(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    // plugin attributes
    const auto ID = "SNDH"_id;
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::AY38910
                        | Module::AYM::GetSupportedFormatConvertors();

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
