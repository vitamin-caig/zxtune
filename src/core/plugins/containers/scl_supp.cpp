/*
Abstract:
  SCL containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "core/plugins/registrator.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/archived_decoders.h>

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'S', 'C', 'L', '\0'};
  const uint_t CAPS = CAP_STOR_MULTITRACK | CAP_STOR_PLAIN;
}

namespace ZXTune
{
  void RegisterSCLContainer(PluginsRegistrator& registrator)
  {
    const Formats::Archived::Decoder::Ptr decoder = Formats::Archived::CreateSCLDecoder();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}
