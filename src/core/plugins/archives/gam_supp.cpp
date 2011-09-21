/*
Abstract:
  GAM convertors support

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
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'G', 'A', 'M', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::GAM_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_CONTAINER;
}

namespace ZXTune
{
  void RegisterGAMConvertor(PluginsRegistrator& registrator)
  {
    Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateGamePackerDecoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID, INFO, VERSION, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}
