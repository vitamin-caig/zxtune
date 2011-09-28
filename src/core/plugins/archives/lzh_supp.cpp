/*
Abstract:
  LZH convertors support

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

  const uint_t CAPS = CAP_STOR_CONTAINER;
}

namespace LZH1
{
  const Char ID[] = {'L', 'Z', 'H', '1', '\0'};
  const Char* const INFO = Text::LZH1_PLUGIN_INFO;

  void RegisterConverter(PluginsRegistrator& registrator)
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateLZH1Decoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID, INFO, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}

namespace LZH2
{
  const Char ID[] = {'L', 'Z', 'H', '2', '\0'};
  const Char* const INFO = Text::LZH2_PLUGIN_INFO;

  void RegisterConverter(PluginsRegistrator& registrator)
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateLZH2Decoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID, INFO, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}

namespace ZXTune
{
  void RegisterLZHConvertor(PluginsRegistrator& registrator)
  {
    LZH1::RegisterConverter(registrator);
    LZH2::RegisterConverter(registrator);
  }
}
