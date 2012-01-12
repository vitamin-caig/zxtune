/*
Abstract:
  Hrust 2.x convertors support

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

  const Char ID1[] = {'H', 'R', 'U', 'S', 'T', '2', '\0'};
  const Char ID3[] = {'H', 'R', 'U', 'S', 'T', '2', '3', '\0'};
  const uint_t CAPS = CAP_STOR_CONTAINER;
}

namespace ZXTune
{
  void RegisterHrust2xConvertor(PluginsRegistrator& registrator)
  {
    //hrust2.1
    {
      const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateHrust21Decoder();
      const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID1, CAPS, decoder);
      registrator.RegisterPlugin(plugin);
    }
    //hrust2.3
    {
      const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateHrust23Decoder();
      const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID3, CAPS, decoder);
      registrator.RegisterPlugin(plugin);
    }
  }
}
