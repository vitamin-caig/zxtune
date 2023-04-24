/**
 *
 * @file
 *
 * @brief  Internal plugins logic implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/archives/packed.h"
#include "core/plugins/archives/plugins.h"
// library includes
#include <core/plugin_attrs.h>
#include <formats/packed/decoders.h>

namespace ZXTune
{
  typedef Formats::Packed::Decoder::Ptr (*CreatePackedDecoderFunc)();

  struct ArchivePluginDescription
  {
    const PluginId Id;
    const CreatePackedDecoderFunc Create;
    const uint_t Caps;
  };

  using namespace Formats::Packed;

  // clang-format off
  const ArchivePluginDescription DEPACKERS[] =
  {
    {"HOBETA"_id,   &CreateHobetaDecoder,                    Capabilities::Container::Type::ARCHIVE | Capabilities::Container::Traits::PLAIN},
    {"HRUST1"_id,   &CreateHrust1Decoder,                    Capabilities::Container::Type::COMPRESSOR},
    {"HRUST2"_id,   &CreateHrust21Decoder,                   Capabilities::Container::Type::COMPRESSOR},
    {"HRUST23"_id,  &CreateHrust23Decoder,                   Capabilities::Container::Type::COMPRESSOR},
    {"FDI"_id,      &CreateFullDiskImageDecoder,             Capabilities::Container::Type::DISKIMAGE},
    {"DSQ"_id,      &CreateDataSquieezerDecoder,             Capabilities::Container::Type::COMPRESSOR},
    {"MSP"_id,      &CreateMSPackDecoder,                    Capabilities::Container::Type::COMPRESSOR},
    {"TRUSH"_id,    &CreateTRUSHDecoder,                     Capabilities::Container::Type::COMPRESSOR},
    {"LZS"_id,      &CreateLZSDecoder,                       Capabilities::Container::Type::COMPRESSOR},
    {"PCD61"_id,    &CreatePowerfullCodeDecreaser61Decoder,  Capabilities::Container::Type::COMPRESSOR},
    {"PCD61i"_id,   &CreatePowerfullCodeDecreaser61iDecoder, Capabilities::Container::Type::COMPRESSOR},
    {"PCD62"_id,    &CreatePowerfullCodeDecreaser62Decoder,  Capabilities::Container::Type::COMPRESSOR},
    {"HRUM"_id,     &CreateHrumDecoder,                      Capabilities::Container::Type::COMPRESSOR},
    {"CC3"_id,      &CreateCodeCruncher3Decoder,             Capabilities::Container::Type::COMPRESSOR},
    {"CC4"_id,      &CreateCompressorCode4Decoder,           Capabilities::Container::Type::COMPRESSOR},
    {"CC4PLUS"_id,  &CreateCompressorCode4PlusDecoder,       Capabilities::Container::Type::COMPRESSOR},
    {"ESV"_id,      &CreateESVCruncherDecoder,               Capabilities::Container::Type::COMPRESSOR},
    {"GAM"_id,      &CreateGamePackerDecoder,                Capabilities::Container::Type::COMPRESSOR},
    {"GAMPLUS"_id,  &CreateGamePackerPlusDecoder,            Capabilities::Container::Type::COMPRESSOR},
    {"TLZ"_id,      &CreateTurboLZDecoder,                   Capabilities::Container::Type::COMPRESSOR},
    {"TLZP"_id,     &CreateTurboLZProtectedDecoder,          Capabilities::Container::Type::COMPRESSOR},
    {"CHARPRES"_id, &CreateCharPresDecoder,                  Capabilities::Container::Type::COMPRESSOR},
    {"PACK2"_id,    &CreatePack2Decoder,                     Capabilities::Container::Type::COMPRESSOR},
    {"LZH1"_id,     &CreateLZH1Decoder,                      Capabilities::Container::Type::COMPRESSOR},
    {"LZH2"_id,     &CreateLZH2Decoder,                      Capabilities::Container::Type::COMPRESSOR},
    {"SNA128"_id,   &CreateSna128Decoder,                    Capabilities::Container::Type::SNAPSHOT},
    {"TD0"_id,      &CreateTeleDiskImageDecoder,             Capabilities::Container::Type::DISKIMAGE},
    {"Z80V145"_id,  &CreateZ80V145Decoder ,                  Capabilities::Container::Type::SNAPSHOT},
    {"Z80V20"_id,   &CreateZ80V20Decoder,                    Capabilities::Container::Type::SNAPSHOT},
    {"Z80V30"_id,   &CreateZ80V30Decoder,                    Capabilities::Container::Type::SNAPSHOT},
    {"MEGALZ"_id,   &CreateMegaLZDecoder,                    Capabilities::Container::Type::COMPRESSOR},
    {"DSK"_id,      &CreateDSKDecoder,                       Capabilities::Container::Type::DISKIMAGE},
  };

  const ArchivePluginDescription CHIPTUNE_PACKERS[] =
  {
    {"GZIP"_id,     &CreateGzipDecoder,                      Capabilities::Container::Type::ARCHIVE},//may contain source filename, so can be treated as archive
    {"MUSE"_id,     &CreateMUSEDecoder,                      Capabilities::Container::Type::COMPRESSOR},
  };

  const ArchivePluginDescription DECOMPILERS[] =
  {
    {"COMPILEDASC0"_id,  &CreateCompiledASC0Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDASC1"_id,  &CreateCompiledASC1Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDASC2"_id,  &CreateCompiledASC2Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDPT24"_id,  &CreateCompiledPT24Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDPTU13"_id, &CreateCompiledPTU13Decoder, Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDST3"_id,   &CreateCompiledST3Decoder,   Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDSTP1"_id,  &CreateCompiledSTP1Decoder,  Capabilities::Container::Type::DECOMPILER},
    {"COMPILEDSTP2"_id,  &CreateCompiledSTP2Decoder,  Capabilities::Container::Type::DECOMPILER},
  };
  // clang-format on

  void RegisterPlugin(const ArchivePluginDescription& desc, ArchivePluginsRegistrator& registrator)
  {
    auto decoder = desc.Create();
    auto plugin = CreateArchivePlugin(desc.Id, desc.Caps, std::move(decoder));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune

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
}  // namespace ZXTune
