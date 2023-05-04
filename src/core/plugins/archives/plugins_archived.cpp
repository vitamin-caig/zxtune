/**
 *
 * @file
 *
 * @brief  Factories of different container plugins
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/archives/archived.h"
#include "core/plugins/archives/plugins.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/archived/decoders.h>
#include <formats/archived/multitrack/decoders.h>

namespace ZXTune
{
  typedef Formats::Archived::Decoder::Ptr (*CreateArchivedDecoderFunc)();

  struct ContainerPluginDescription
  {
    const PluginId Id;
    const CreateArchivedDecoderFunc Create;
    const uint_t Caps;
  };

  using namespace Formats::Archived;

  // clang-format off
  const ContainerPluginDescription UNARCHIVES[] =
  {
    {"ZIP"_id,     &CreateZipDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
    {"RAR"_id,     &CreateRarDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
    {"LHA"_id,     &CreateLhaDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
    {"UMX"_id,     &CreateUMXDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::PLAIN},
    {"7ZIP"_id,    &Create7zipDecoder,    Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
  };

  const ContainerPluginDescription ZXUNARCHIVES[] =
  {
    {"TRD"_id,     &CreateTRDDecoder,     Capabilities::Container::Type::DISKIMAGE | Capabilities::Container::Traits::PLAIN},
    {"SCL"_id,     &CreateSCLDecoder,     Capabilities::Container::Type::DISKIMAGE | Capabilities::Container::Traits::PLAIN},
    {"HRIP"_id,    &CreateHripDecoder,    Capabilities::Container::Type::ARCHIVE},
    {"ZXZIP"_id,   &CreateZXZipDecoder,   Capabilities::Container::Type::ARCHIVE},
    {"ZXSTATE"_id, &CreateZXStateDecoder, Capabilities::Container::Type::SNAPSHOT},
  };

  const ContainerPluginDescription MULTITRACKS[] =
  {
    {"AY"_id,      &CreateAYDecoder,      Capabilities::Container::Type::MULTITRACK},
  };
  // clang-format on

  void RegisterPlugin(const ContainerPluginDescription& desc, ArchivePluginsRegistrator& registrator)
  {
    auto decoder = desc.Create();
    auto plugin = CreateArchivePlugin(desc.Id, desc.Caps, std::move(decoder));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune

namespace ZXTune
{
  void RegisterMultitrackContainers(ArchivePluginsRegistrator& registrator)
  {
    for (const auto& desc : MULTITRACKS)
    {
      RegisterPlugin(desc, registrator);
    }
  }

  void RegisterArchiveContainers(ArchivePluginsRegistrator& registrator)
  {
    for (const auto& desc : UNARCHIVES)
    {
      RegisterPlugin(desc, registrator);
    }
  }

  void RegisterZXArchiveContainers(ArchivePluginsRegistrator& registrator)
  {
    for (const auto& desc : ZXUNARCHIVES)
    {
      RegisterPlugin(desc, registrator);
    }
  }
}  // namespace ZXTune
