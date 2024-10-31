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
    auto decoder = Formats::Chiptune::YM::CreateVTXDecoder();
    auto factory = Module::YMVTX::CreateFactory(decoder);
    auto plugin = CreateStreamPlayerPlugin("VTX"_id, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }

  void RegisterYMSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "YM"_id;
    {
      auto decoder = Formats::Chiptune::YM::CreatePackedYMDecoder();
      auto factory = Module::YMVTX::CreateFactory(decoder);
      auto plugin = CreateStreamPlayerPlugin(ID, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      auto decoder = Formats::Chiptune::YM::CreateYMDecoder();
      auto factory = Module::YMVTX::CreateFactory(decoder);
      auto plugin = CreateStreamPlayerPlugin(ID, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
