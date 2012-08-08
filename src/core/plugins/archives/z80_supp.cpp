/*
Abstract:
  Z80 convertors support

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

  const Char ID145[] = {'Z', '8', '0', 'V', '1', '4', '5', '\0'};
  const Char ID20[] = {'Z', '8', '0', 'V', '2', '0', '\0'};
  const Char ID30[] = {'Z', '8', '0', 'V', '3', '0', '\0'};
  const uint_t CAPS = CAP_STOR_CONTAINER;
}

namespace ZXTune
{
  void RegisterZ80Convertor(PluginsRegistrator& registrator)
  {
    {
      const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateZ80V145Decoder();
      const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID145, CAPS, decoder);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateZ80V20Decoder();
      const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID20, CAPS, decoder);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateZ80V30Decoder();
      const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID30, CAPS, decoder);
      registrator.RegisterPlugin(plugin);
    }
  }
}
