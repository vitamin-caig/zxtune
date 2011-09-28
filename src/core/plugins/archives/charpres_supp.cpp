/*
Abstract:
  CharPres convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "archive_supp_common.h"
#include <core/plugins/registrator.h>
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed_decoders.h>

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'C', 'H', 'A', 'R', 'P', 'R', 'E', 'S', '\0'};
  const uint_t CAPS = CAP_STOR_CONTAINER;
}

namespace ZXTune
{
  void RegisterCharPresConvertor(PluginsRegistrator& registrator)
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateCharPresDecoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}
