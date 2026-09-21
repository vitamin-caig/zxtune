/**
 *
 * @file
 *
 * @brief  SPC support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/emulation/spc.h"
#include "module/players/external/spc.h"

#include "core/plugin_attrs.h"

#include "make_ptr.h"
#include "string_view.h"

namespace ZXTune
{
  void RegisterSPCSupport(PlayerPluginsRegistrator& registrator)
  {
    const auto ID = "SPC"_id;
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::SPC700;

    auto decoder = Formats::Chiptune::CreateSPCDecoder();
    auto factory = Module::SPC::CreateFactory();
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
