/**
* 
* @file
*
* @brief  Factories of different container plugins
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container_supp_common.h"
#include "plugins.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/archived/decoders.h>
#include <formats/archived/multitrack/decoders.h>
//boost includes
#include <boost/range/end.hpp>

namespace
{
  typedef Formats::Archived::Decoder::Ptr (*CreateDecoderFunc)();

  struct ContainerPluginDescription
  {
    const char* const Id;
    const CreateDecoderFunc Create;
    const uint_t Caps;
  };

  using namespace ZXTune;
  using namespace Formats::Archived;

  const ContainerPluginDescription UNARCHIVES[] =
  {
    {"TRD",     &CreateTRDDecoder,     Capabilities::Container::Type::DISKIMAGE | Capabilities::Container::Traits::PLAIN},
    {"SCL",     &CreateSCLDecoder,     Capabilities::Container::Type::DISKIMAGE | Capabilities::Container::Traits::PLAIN},
    {"HRIP",    &CreateHripDecoder,    Capabilities::Container::Type::ARCHIVE},
    {"ZXZIP",   &CreateZXZipDecoder,   Capabilities::Container::Type::ARCHIVE},
    {"ZIP",     &CreateZipDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
    {"RAR",     &CreateRarDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
    {"LHA",     &CreateLhaDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::DIRECTORIES},
    {"ZXSTATE", &CreateZXStateDecoder, Capabilities::Container::Type::SNAPSHOT},
    {"UMX",     &CreateUMXDecoder,     Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::PLAIN},
  };

  const ContainerPluginDescription MULTITRACKS[] =
  {
    {"AY",      &CreateAYDecoder,      Capabilities::Container::Type::MULTITRACK},
    {"SID",     &CreateSIDDecoder,     Capabilities::Container::Type::MULTITRACK | Capabilities::Container::Traits::ONCEAPPLIED},
    {"NSF",     &CreateNSFDecoder,     Capabilities::Container::Type::MULTITRACK | Capabilities::Container::Traits::ONCEAPPLIED},
    {"NSFE",    &CreateNSFEDecoder,    Capabilities::Container::Type::MULTITRACK | Capabilities::Container::Traits::ONCEAPPLIED},
    {"GBS",     &CreateGBSDecoder,     Capabilities::Container::Type::MULTITRACK | Capabilities::Container::Traits::ONCEAPPLIED},
    {"SAP",     &CreateSAPDecoder,     Capabilities::Container::Type::MULTITRACK | Capabilities::Container::Traits::ONCEAPPLIED},
  };

  void RegisterPlugin(const ContainerPluginDescription& desc, ArchivePluginsRegistrator& registrator)
  {
    const Formats::Archived::Decoder::Ptr decoder = desc.Create();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(FromStdString(desc.Id), desc.Caps, decoder);
    registrator.RegisterPlugin(plugin);
  }
}

namespace ZXTune
{
  void RegisterMultitrackContainers(ArchivePluginsRegistrator& registrator)
  {
    for (const ContainerPluginDescription* it = MULTITRACKS; it != boost::end(MULTITRACKS); ++it)
    {
      RegisterPlugin(*it, registrator);
    }
  }

  void RegisterArchiveContainers(ArchivePluginsRegistrator& registrator)
  {
    for (const ContainerPluginDescription* it = UNARCHIVES; it != boost::end(UNARCHIVES); ++it)
    {
      RegisterPlugin(*it, registrator);
    }
  }
}
