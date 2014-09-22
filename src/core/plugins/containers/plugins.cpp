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
    {"TRD",     &CreateTRDDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_PLAIN},
    {"SCL",     &CreateSCLDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_PLAIN},
    {"HRIP",    &CreateHripDecoder,    CAP_STOR_MULTITRACK},
    {"ZXZIP",   &CreateZXZipDecoder,   CAP_STOR_MULTITRACK},
    {"ZIP",     &CreateZipDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_DIRS},
    {"RAR",     &CreateRarDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_DIRS},
    {"LHA",     &CreateLhaDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_DIRS},
    {"ZXSTATE", &CreateZXStateDecoder, CAP_STOR_MULTITRACK},
  };

  const ContainerPluginDescription AY =
    {"AY",      &CreateAYDecoder,      CAP_STOR_MULTITRACK};

  void RegisterPlugin(const ContainerPluginDescription& desc, ArchivePluginsRegistrator& registrator)
  {
    const Formats::Archived::Decoder::Ptr decoder = desc.Create();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(FromStdString(desc.Id), desc.Caps, decoder);
    registrator.RegisterPlugin(plugin);
  }
}

namespace ZXTune
{
  void RegisterAyContainer(ArchivePluginsRegistrator& registrator)
  {
    RegisterPlugin(AY, registrator);
  }

  void RegisterArchiveContainers(ArchivePluginsRegistrator& registrator)
  {
    for (const ContainerPluginDescription* it = UNARCHIVES; it != boost::end(UNARCHIVES); ++it)
    {
      RegisterPlugin(*it, registrator);
    }
  }
}
