/*
Abstract:
  Archive plugins list implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "archive_supp_common.h"
#include "plugins_list.h"
#include <core/plugins/registrator.h>
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed_decoders.h>

namespace
{
  typedef Formats::Packed::Decoder::Ptr (*CreateDecoderFunc)();

  struct ArchivePluginDescription
  {
    const char* const Id;
    const CreateDecoderFunc Create;
    const uint_t Caps;
  };

  using namespace ZXTune;
  using namespace Formats::Packed;

  const ArchivePluginDescription PLUGINS[] =
  {
    {"HOBETA",   &CreateHobetaDecoder,                   CAP_STOR_CONTAINER | CAP_STOR_PLAIN},
    {"HRUST1",   &CreateHrust1Decoder,                   CAP_STOR_CONTAINER},
    {"HRUST2",   &CreateHrust21Decoder,                  CAP_STOR_CONTAINER},
    {"HRUST23",  &CreateHrust23Decoder,                  CAP_STOR_CONTAINER},
    {"FDI",      &CreateFullDiskImageDecoder,            CAP_STOR_CONTAINER},
    {"DSQ",      &CreateDataSquieezerDecoder,            CAP_STOR_CONTAINER},
    {"MSP",      &CreateMSPackDecoder,                   CAP_STOR_CONTAINER},
    {"TRUSH",    &CreateTRUSHDecoder,                    CAP_STOR_CONTAINER},
    {"LZS",      &CreateLZSDecoder,                      CAP_STOR_CONTAINER},
    {"PCD61",    &CreatePowerfullCodeDecreaser61Decoder, CAP_STOR_CONTAINER},
    {"PCD62",    &CreatePowerfullCodeDecreaser62Decoder, CAP_STOR_CONTAINER},
    {"HRUM",     &CreateHrumDecoder,                     CAP_STOR_CONTAINER},
    {"CC3",      &CreateCodeCruncher3Decoder,            CAP_STOR_CONTAINER},
    {"CC4",      &CreateCompressorCode4Decoder,          CAP_STOR_CONTAINER},
    {"CC4PLUS",  &CreateCompressorCode4PlusDecoder,      CAP_STOR_CONTAINER},
    {"ESV",      &CreateESVCruncherDecoder,              CAP_STOR_CONTAINER},
    {"GAM",      &CreateGamePackerDecoder,               CAP_STOR_CONTAINER},
    {"GAMPLUS",  &CreateGamePackerPlusDecoder,           CAP_STOR_CONTAINER},
    {"TLZ",      &CreateTurboLZDecoder,                  CAP_STOR_CONTAINER},
    {"TLZP",     &CreateTurboLZProtectedDecoder,         CAP_STOR_CONTAINER},
    {"CHARPRES", &CreateCharPresDecoder,                 CAP_STOR_CONTAINER},
    {"PACK2",    &CreatePack2Decoder,                    CAP_STOR_CONTAINER},
    {"LZH1",     &CreateLZH1Decoder,                     CAP_STOR_CONTAINER},
    {"LZH2",     &CreateLZH2Decoder,                     CAP_STOR_CONTAINER},
    {"SNA128",   &CreateSna128Decoder,                   CAP_STOR_CONTAINER},
    {"TD0",      &CreateTeleDiskImageDecoder,            CAP_STOR_CONTAINER},
    {"Z80V145",  &CreateZ80V145Decoder ,                 CAP_STOR_CONTAINER},
    {"Z80V20",   &CreateZ80V20Decoder,                   CAP_STOR_CONTAINER},
    {"Z80V30",   &CreateZ80V30Decoder,                   CAP_STOR_CONTAINER},
    {"MEGALZ",   &CreateMegaLZDecoder,                   CAP_STOR_CONTAINER}
  };
}

namespace ZXTune
{
  void RegisterArchivePlugins(PluginsRegistrator& registrator)
  {
    for (const ArchivePluginDescription* it = PLUGINS; it != ArrayEnd(PLUGINS); ++it)
    {
      const ArchivePluginDescription& desc = *it;
      const Formats::Packed::Decoder::Ptr decoder = desc.Create();
      const ArchivePlugin::Ptr plugin = CreateArchivePlugin(FromStdString(desc.Id), desc.Caps, decoder);
      registrator.RegisterPlugin(plugin);
    }
  }
}
