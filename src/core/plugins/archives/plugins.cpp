/**
* 
* @file
*
* @brief  Internal plugins logic implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "archive_supp_common.h"
#include "plugins.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed/decoders.h>
//boost includes
#include <boost/range/end.hpp>

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

  const ArchivePluginDescription DEPACKERS[] =
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
    {"MEGALZ",   &CreateMegaLZDecoder,                   CAP_STOR_CONTAINER},
  };

  const ArchivePluginDescription DECOMPILERS[] =
  {
    {"COMPILEDASC0",  &CreateCompiledASC0Decoder,  CAP_STOR_CONTAINER},
    {"COMPILEDASC1",  &CreateCompiledASC1Decoder,  CAP_STOR_CONTAINER},
    {"COMPILEDASC2",  &CreateCompiledASC2Decoder,  CAP_STOR_CONTAINER},
    {"COMPILEDPT24",  &CreateCompiledPT24Decoder,  CAP_STOR_CONTAINER},
    {"COMPILEDPTU13", &CreateCompiledPTU13Decoder, CAP_STOR_CONTAINER},
    {"COMPILEDST3",   &CreateCompiledST3Decoder,   CAP_STOR_CONTAINER},
    {"COMPILEDSTP1",  &CreateCompiledSTP1Decoder,  CAP_STOR_CONTAINER},
    {"COMPILEDSTP2",  &CreateCompiledSTP2Decoder,  CAP_STOR_CONTAINER},
  };

  void RegisterPlugins(const ArchivePluginDescription* from, const ArchivePluginDescription* to,
    ArchivePluginsRegistrator& registrator)
  {
    for (const ArchivePluginDescription* it = from; it != to; ++it)
    {
      const ArchivePluginDescription& desc = *it;
      const Formats::Packed::Decoder::Ptr decoder = desc.Create();
      const ArchivePlugin::Ptr plugin = CreateArchivePlugin(FromStdString(desc.Id), desc.Caps, decoder);
      registrator.RegisterPlugin(plugin);
    }
  }
}

namespace ZXTune
{
  void RegisterDepackPlugins(ArchivePluginsRegistrator& registrator)
  {
    RegisterPlugins(DEPACKERS, boost::end(DEPACKERS), registrator);
  }

  void RegisterDecompilePlugins(ArchivePluginsRegistrator& registrator)
  {
    RegisterPlugins(DECOMPILERS, boost::end(DECOMPILERS), registrator);
  }
}
