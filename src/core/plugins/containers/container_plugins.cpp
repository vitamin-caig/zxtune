/*
Abstract:
  Containers plugins list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "plugins_list.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/archived/decoders.h>

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

  const ContainerPluginDescription PLUGINS[] =
  {
    {"TRD",     &CreateTRDDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_PLAIN},
    {"SCL",     &CreateSCLDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_PLAIN},
    {"HRIP",    &CreateHripDecoder,    CAP_STOR_MULTITRACK},
    {"ZXZIP",   &CreateZXZipDecoder,   CAP_STOR_MULTITRACK},
    {"ZIP",     &CreateZipDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_DIRS},
    {"RAR",     &CreateRarDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_DIRS},
    {"LHA",     &CreateLhaDecoder,     CAP_STOR_MULTITRACK | CAP_STOR_DIRS},
    {"ZXSTATE", &CreateZXStateDecoder, CAP_STOR_MULTITRACK},
    {"AY",      &CreateAYDecoder,      CAP_STOR_MULTITRACK},
  };
}

namespace ZXTune
{
  void RegisterRawContainer(ArchivePluginsRegistrator& registrator);

  void RegisterContainerPlugins(ArchivePluginsRegistrator& registrator)
  {
    //process raw container first
    RegisterRawContainer(registrator);

    for (const ContainerPluginDescription* it = PLUGINS; it != ArrayEnd(PLUGINS); ++it)
    {
      const ContainerPluginDescription& desc = *it;
      const Formats::Archived::Decoder::Ptr decoder = desc.Create();
      const ArchivePlugin::Ptr plugin = CreateContainerPlugin(FromStdString(desc.Id), desc.Caps, decoder);
      registrator.RegisterPlugin(plugin);
    }
  }
}
