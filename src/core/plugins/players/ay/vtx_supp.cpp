/**
 *
 * @file
 *
 * @brief  YM/VTX support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/ay/aym_plugin.h"
#include "module/players/aym/ymvtx.h"

namespace ZXTune
{
  void RegisterVTXSupport(PlayerPluginsRegistrator& registrator)
  {
    using namespace Formats::Chiptune::YM;
    auto decoder = CreateVTXDecoder();
    auto factory = Module::YMVTX::CreateFactory(ParseVTX);
    auto plugin = CreateStreamPlayerPlugin("VTX"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }

  void RegisterYMSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "YM"_id;
    using namespace Formats::Chiptune::YM;
    {
      auto decoder = CreatePackedYMDecoder();
      auto factory = Module::YMVTX::CreateFactory(ParsePacked);
      auto plugin = CreateStreamPlayerPlugin(ID, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = CreateYMDecoder();
      auto factory = Module::YMVTX::CreateFactory(Parse);
      auto plugin = CreateStreamPlayerPlugin(ID, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
