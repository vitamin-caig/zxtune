/**
 *
 * @file
 *
 * @brief  XMP support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "module/players/external/xmp.h"

#include "core/plugin_attrs.h"

namespace ZXTune::XMP
{
  struct PluginDescription
  {
    using ChiptuneDecoderCreator = Formats::Chiptune::Decoder::Ptr (*)();
    using FactoryCreator = Module::ExternalParsingFactory::Ptr (*)();

    const PluginId Id;
    const ChiptuneDecoderCreator CreateDecoder;
    const FactoryCreator CreateFactory;
  };

  // clang-format off
  const PluginDescription PLUGINS[] =
  {
    {
      "DTT"_id,
      &Formats::Chiptune::CreateDTTDecoder,
      &Module::Xmp::CreateDTTFactory,
    },
    {
      "EMOD"_id,
      &Formats::Chiptune::CreateEMODDecoder,
      &Module::Xmp::CreateEMODFactory,
    },
    {
      "FNK"_id,
      &Formats::Chiptune::CreateFNKDecoder,
      &Module::Xmp::CreateFNKFactory,
    },
    {
      "LIQ"_id,
      &Formats::Chiptune::CreateLIQDecoder,
      &Module::Xmp::CreateLIQFactory,
    },
    {
      "MED"_id,
      &Formats::Chiptune::CreateMED2Decoder,
      &Module::Xmp::CreateMED2Factory,
    },
    {
      "MED"_id,
      &Formats::Chiptune::CreateMED3Decoder,
      &Module::Xmp::CreateMED3Factory,
    },
    {
      "MED"_id,
      &Formats::Chiptune::CreateMED4Decoder,
      &Module::Xmp::CreateMED4Factory,
    },
    {
      "LIQ"_id,
      &Formats::Chiptune::CreateNODecoder,
      &Module::Xmp::CreateNOFactory,
    },
    {
      "STIM"_id,
      &Formats::Chiptune::CreateSTIMDecoder,
      &Module::Xmp::CreateSTIMFactory,
    },
    {
      "STX"_id,
      &Formats::Chiptune::CreateSTXDecoder,
      &Module::Xmp::CreateSTXFactory,
    },
  };
  // clang-format on
}

namespace ZXTune
{
  void RegisterXMPPlugins(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;
    for (const auto& desc : XMP::PLUGINS)
    {
      auto decoder = desc.CreateDecoder();
      auto factory = desc.CreateFactory();
      auto plugin = CreatePlayerPlugin(desc.Id, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
