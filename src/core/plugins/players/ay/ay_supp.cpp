/**
 *
 * @file
 *
 * @brief  AY modules support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_conversion.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/emulation/ay.h"
#include "module/players/aym/ayemul.h"

#include "core/plugin_attrs.h"

#include "types.h"

#include <utility>

namespace ZXTune
{
  void RegisterAYSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "AY"_id;
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::AY38910
                        | Capabilities::Module::Device::BEEPER | Module::AYM::GetSupportedFormatConvertors();

    auto decoder = Formats::Chiptune::CreateAYEMULDecoder();
    auto factory = Module::AYEMUL::CreateFactory();
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
