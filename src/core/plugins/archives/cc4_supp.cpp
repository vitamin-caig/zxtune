/*
Abstract:
  CompressorCode v3 convertors support

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

  const Char ID[] = {'C', 'C', '4', '\0'};
  const Char IDPLUS[] = {'C', 'C', '4', 'P', 'L', 'U', 'S', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::CC4_PLUGIN_INFO;
  const Char* const INFOPLUS = Text::CC4PLUS_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_CONTAINER;

  void RegisterCC4Support(PluginsRegistrator& registrator)
  {
    Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateCompressorCode4Decoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID, INFO, VERSION, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }

  void RegisterCC4PlusSupport(PluginsRegistrator& registrator)
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateCompressorCode4PlusDecoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(IDPLUS, INFOPLUS, VERSION, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}

namespace ZXTune
{
  void RegisterCC4Convertor(PluginsRegistrator& registrator)
  {
    RegisterCC4Support(registrator);
    RegisterCC4PlusSupport(registrator);
  }
}
