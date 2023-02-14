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
    const char* const Id;
    const CreateArchivedDecoderFunc Create;
    const uint_t Caps;
  };

  using namespace Formats::Archived;

  // clang-format off
  const ContainerPluginDescription UNARCHIVES[] =
  {
    {"ZIP",     &CreateZipDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
    {"RAR",     &CreateRarDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
    {"LHA",     &CreateLhaDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
    {"UMX",     &CreateUMXDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::PLAIN},
    {"7ZIP",    &Create7zipDecoder,    Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
  };

  const ContainerPluginDescription ZXUNARCHIVES[] =
  {
    {"TRD",     &CreateTRDDecoder,     Capabilities::Container::Type::DISKIMAGE | Capabilities::Container::Traits::PLAIN},
    {"SCL",     &CreateSCLDecoder,     Capabilities::Container::Type::DISKIMAGE | Capabilities::Container::Traits::PLAIN},
    {"HRIP",    &CreateHripDecoder,    Capabilities::Container::Type::ARCHIVE},
    {"ZXZIP",   &CreateZXZipDecoder,   Capabilities::Container::Type::ARCHIVE},
    {"ZXSTATE", &CreateZXStateDecoder, Capabilities::Container::Type::SNAPSHOT},
  };

  const ContainerPluginDescription MULTITRACKS[] =
  {
    {"AY",      &CreateAYDecoder,      Capabilities::Container::Type::MULTITRACK},
  };
  // clang-format on

  void RegisterPlugin(const ContainerPluginDescription& desc, ArchivePluginsRegistrator& registrator)
  {
    const Formats::Archived::Decoder::Ptr decoder = desc.Create();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(desc.Id, desc.Caps, decoder);
    registrator.RegisterPlugin(plugin);
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
