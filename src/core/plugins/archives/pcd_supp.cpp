/*
Abstract:
  PCD convertors support

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

  const Char ID61[] = {'P', 'C', 'D', '6', '1', '\0'};
  const Char ID62[] = {'P', 'C', 'D', '6', '2', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO61 = Text::PCD61_PLUGIN_INFO;
  const Char* const INFO62 = Text::PCD62_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_CONTAINER;

  void RegisterPCD61Support(PluginsRegistrator& registrator)
  {
    Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreatePowerfullCodeDecreaser61Decoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID61, INFO61, VERSION, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }

  void RegisterPCD62Support(PluginsRegistrator& registrator)
  {
    Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreatePowerfullCodeDecreaser62Decoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID62, INFO62, VERSION, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}

namespace ZXTune
{
  void RegisterPCDConvertor(PluginsRegistrator& registrator)
  {
    RegisterPCD61Support(registrator);
    RegisterPCD62Support(registrator);
  }
}
