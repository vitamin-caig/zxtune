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
#include "core/plugins/archives/packed.h"
#include "core/plugins/archives/plugins.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed/decoders.h>

namespace ZXTune
{
  typedef Formats::Packed::Decoder::Ptr (*CreatePackedDecoderFunc)();

  struct ArchivePluginDescription
  {
    const char* const Id;
    const CreatePackedDecoderFunc Create;
    const uint_t Caps;
  };

  using namespace Formats::Packed;

  const ArchivePluginDescription DEPACKERS[] =
  {
    {"HOBETA",   &CreateHobetaDecoder,                    Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::PLAIN},
    {"HRUST1",   &CreateHrust1Decoder,                    Capabilities::Container::Type::COMPRESSOR},
    {"HRUST2",   &CreateHrust21Decoder,                   Capabilities::Container::Type::COMPRESSOR},
    {"HRUST23",  &CreateHrust23Decoder,                   Capabilities::Container::Type::COMPRESSOR},
    {"FDI",      &CreateFullDiskImageDecoder,             Capabilities::Container::Type::DISKIMAGE},
    {"DSQ",      &CreateDataSquieezerDecoder,             Capabilities::Container::Type::COMPRESSOR},
    {"MSP",      &CreateMSPackDecoder,                    Capabilities::Container::Type::COMPRESSOR},
    {"TRUSH",    &CreateTRUSHDecoder,                     Capabilities::Container::Type::COMPRESSOR},
    {"LZS",      &CreateLZSDecoder,                       Capabilities::Container::Type::COMPRESSOR},
    {"PCD61",    &CreatePowerfullCodeDecreaser61Decoder,  Capabilities::Container::Type::COMPRESSOR},
    {"PCD61i",   &CreatePowerfullCodeDecreaser61iDecoder, Capabilities::Container::Type::COMPRESSOR},
    {"PCD62",    &CreatePowerfullCodeDecreaser62Decoder,  Capabilities::Container::Type::COMPRESSOR},
    {"HRUM",     &CreateHrumDecoder,                      Capabilities::Container::Type::COMPRESSOR},
    {"CC3",      &CreateCodeCruncher3Decoder,             Capabilities::Container::Type::COMPRESSOR},
    {"CC4",      &CreateCompressorCode4Decoder,           Capabilities::Container::Type::COMPRESSOR},
    {"CC4PLUS",  &CreateCompressorCode4PlusDecoder,       Capabilities::Container::Type::COMPRESSOR},
    {"ESV",      &CreateESVCruncherDecoder,               Capabilities::Container::Type::COMPRESSOR},
    {"GAM",      &CreateGamePackerDecoder,                Capabilities::Container::Type::COMPRESSOR},
    {"GAMPLUS",  &CreateGamePackerPlusDecoder,            Capabilities::Container::Type::COMPRESSOR},
    {"TLZ",      &CreateTurboLZDecoder,                   Capabilities::Container::Type::COMPRESSOR},
    {"TLZP",     &CreateTurboLZProtectedDecoder,          Capabilities::Container::Type::COMPRESSOR},
    {"CHARPRES", &CreateCharPresDecoder,                  Capabilities::Container::Type::COMPRESSOR},
    {"PACK2",    &CreatePack2Decoder,                     Capabilities::Container::Type::COMPRESSOR},
    {"LZH1",     &CreateLZH1Decoder,                      Capabilities::Container::Type::COMPRESSOR},
    {"LZH2",     &CreateLZH2Decoder,                      Capabilities::Container::Type::COMPRESSOR},
    {"SNA128",   &CreateSna128Decoder,                    Capabilities::Container::Type::SNAPSHOT},
    {"TD0",      &CreateTeleDiskImageDecoder,             Capabilities::Container::Type::DISKIMAGE},
    {"Z80V145",  &CreateZ80V145Decoder ,                  Capabilities::Container::Type::SNAPSHOT},
    {"Z80V20",   &CreateZ80V20Decoder,                    Capabilities::Container::Type::SNAPSHOT},
    {"Z80V30",   &CreateZ80V30Decoder,                    Capabilities::Container::Type::SNAPSHOT},
    {"MEGALZ",   &CreateMegaLZDecoder,                    Capabilities::Container::Type::COMPRESSOR},
    {"DSK",      &CreateDSKDecoder,                       Capabilities::Container::Type::DISKIMAGE},
  };

  const ArchivePluginDescription CHIPTUNE_PACKERS[] =
  {
    {"GZIP",     &CreateGzipDecoder,                      Capabilities::Container::Type::ARCHIVE},//may contain source filename, so can be treated as archive
  };

  const ArchivePluginDescription DECOMPILERS[] =
  {
    {"COMPILEDASC0",  &CreateCompiledASC0Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDASC1",  &CreateCompiledASC1Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDASC2",  &CreateCompiledASC2Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDPT24",  &CreateCompiledPT24Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDPTU13", &CreateCompiledPTU13Decoder, Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDST3",   &CreateCompiledST3Decoder,   Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDSTP1",  &CreateCompiledSTP1Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDSTP2",  &CreateCompiledSTP2Decoder,  Capabilities::Container::Type::DECOMPILER},
  };

  void RegisterPlugin(const ArchivePluginDescription& desc, ArchivePluginsRegistrator& registrator)
  {
    const Formats::Packed::Decoder::Ptr decoder = desc.Create();
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(FromStdString(desc.Id), desc.Caps, decoder);
    registrator.RegisterPlugin(plugin);
  }
}

namespace ZXTune
{
  void RegisterDepackPlugins(ArchivePluginsRegistrator& registrator)
  {
    for (const auto& desc : DEPACKERS)
    {
      RegisterPlugin(desc, registrator);
    }
  }

  void RegisterChiptunePackerPlugins(ArchivePluginsRegistrator& registrator)
  {
    for (const auto& desc : CHIPTUNE_PACKERS)
    {
      RegisterPlugin(desc, registrator);
    }
  }

  void RegisterDecompilePlugins(ArchivePluginsRegistrator& registrator)
  {
    for (const auto& desc : DECOMPILERS)
    {
      RegisterPlugin(desc, registrator);
    }
  }
}
