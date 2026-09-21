/**
 *
 * @file
 *
 * @brief  XSF-based player plugins factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "module/players/xsf/all.h"

#include "core/plugin_attrs.h"
#include "formats/chiptune/decoders.h"

#include <utility>

namespace ZXTune::XSF
{
  struct PluginDescription
  {
    using DecoderCreator = Formats::Chiptune::Decoder::Ptr (*)();
    using ModuleCreator = Module::XSF::Factory;

    const PluginId Id;
    const DecoderCreator CreateDecoder;
    const ModuleCreator CreateModule;
  };

  // clang-format off
  const PluginDescription PLUGINS[] =
  {
    { "PSF"_id,  &Formats::Chiptune::CreatePSFDecoder,  &Module::XSF::CreatePSFModule },
    { "PSF2"_id, &Formats::Chiptune::CreatePSF2Decoder, &Module::XSF::CreatePSFModule },
    { "USF"_id,  &Formats::Chiptune::CreateUSFDecoder,  &Module::XSF::CreateUSFModule },
    { "GSF"_id,  &Formats::Chiptune::CreateGSFDecoder,  &Module::XSF::CreateGSFModule },
    { "2SF"_id,  &Formats::Chiptune::Create2SFDecoder,  &Module::XSF::Create2SFModule },
    { "NCSF"_id, &Formats::Chiptune::CreateNCSFDecoder, &Module::XSF::CreateNCSFModule },
    { "SSF"_id,  &Formats::Chiptune::CreateSSFDecoder,  &Module::XSF::CreateSDSFModule },
    { "DSF"_id,  &Formats::Chiptune::CreateDSFDecoder,  &Module::XSF::CreateSDSFModule },
  };
  // clang-format on
}

namespace ZXTune
{
  void RegisterXSFPlugins(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::DAC
                        | Capabilities::Module::Traits::MULTIFILE;
    for (const auto& desc : XSF::PLUGINS)
    {
      auto decoder = desc.CreateDecoder();
      auto factory = Module::XSF::CreateModuleFactory(desc.CreateModule);
      auto plugin = CreatePlayerPlugin(desc.Id, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
