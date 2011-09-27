/*
Abstract:
  Trush convertors support

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

  const Char ID[] = {'T', 'R', 'U', 'S', 'H', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::TRUSH_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_CONTAINER;
}

namespace ZXTune
{
  void RegisterTRUSHConvertor(PluginsRegistrator& registrator)
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateTRUSHDecoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID, INFO, VERSION, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}
