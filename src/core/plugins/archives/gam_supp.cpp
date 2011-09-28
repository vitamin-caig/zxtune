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

namespace
{
  using namespace ZXTune;

  const uint_t CAPS = CAP_STOR_CONTAINER;
}

namespace GAM
{
  const Char ID[] = {'G', 'A', 'M', '\0'};

  void RegisterConvertor(PluginsRegistrator& registrator)
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateGamePackerDecoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}

namespace GAMPlus
{
  const Char ID[] = {'G', 'A', 'M', 'P', 'L', 'U', 'S', '\0'};

  void RegisterConvertor(PluginsRegistrator& registrator)
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateGamePackerPlusDecoder();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}

namespace ZXTune
{
  void RegisterGAMConvertor(PluginsRegistrator& registrator)
  {
    GAM::RegisterConvertor(registrator);
    GAMPlus::RegisterConvertor(registrator);
  }
}
