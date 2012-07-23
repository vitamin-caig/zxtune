/*
Abstract:
  ZX State convertors support

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

  const Char ID[] = {'Z', 'X', 'S', 'T', 'A', 'T', 'E', '\0'};
  const uint_t CAPS = CAP_STOR_MULTITRACK;
}

namespace ZXTune
{
  void RegisterZXStateContainer(PluginsRegistrator& registrator)
  {
    const Formats::Archived::Decoder::Ptr decoder = Formats::Archived::CreateZXStateDecoder();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}
