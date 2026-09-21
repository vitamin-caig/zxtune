/**
 *
 * @file
 *
 * @brief  libvgm-based formats support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "module/players/external/libvgm.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"

#include "make_ptr.h"
#include "string_view.h"

namespace ZXTune::VGM
{
  struct SingletrackPluginDescription
  {
    using ChiptuneDecoderCreator = Formats::Chiptune::Decoder::Ptr (*)();
    using SingletrackFactoryCreator = Module::Factory::Ptr (*)();

    const PluginId Id;
    const uint_t ChiptuneCaps;
    const ChiptuneDecoderCreator CreateChiptuneDecoder;
    const SingletrackFactoryCreator CreateSingletrackFactory;
  };

  // clang-format off
  const SingletrackPluginDescription PLUGINS[] =
  {
    {
      "VGM"_id,
      Capabilities::Module::Type::STREAM | Capabilities::Module::Device::MULTI,
      &Formats::Chiptune::CreateVideoGameMusicDecoder,
      &Module::VideoGameMusic::CreateFactory,
    },
    {
      "S98"_id,
      Capabilities::Module::Type::STREAM | Capabilities::Module::Device::MULTI,
      &Formats::Chiptune::CreateSound98Decoder,
      &Module::Sound98::CreateFactory,
    },
  };
  // clang-format on
}

namespace ZXTune
{
  void RegisterVGMPlugins(PlayerPluginsRegistrator& registrator)
  {
    for (const auto& desc : VGM::PLUGINS)
    {
      auto decoder = desc.CreateChiptuneDecoder();
      auto factory = desc.CreateSingletrackFactory();
      auto plugin = CreatePlayerPlugin(desc.Id, desc.ChiptuneCaps, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
