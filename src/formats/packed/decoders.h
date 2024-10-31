/**
 *
 * @file
 *
 * @brief  Packe data decoders factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/packed.h"

namespace Formats::Packed
{
  Decoder::Ptr CreateCodeCruncher3Decoder();
  Decoder::Ptr CreateCompressorCode4Decoder();
  Decoder::Ptr CreateCompressorCode4PlusDecoder();
  Decoder::Ptr CreateDataSquieezerDecoder();
  Decoder::Ptr CreateESVCruncherDecoder();
  Decoder::Ptr CreateHrumDecoder();
  Decoder::Ptr CreateHrust1Decoder();
  Decoder::Ptr CreateHrust21Decoder();
  Decoder::Ptr CreateHrust23Decoder();
  Decoder::Ptr CreateLZSDecoder();
  Decoder::Ptr CreateMSPackDecoder();
  Decoder::Ptr CreatePowerfullCodeDecreaser61Decoder();
  Decoder::Ptr CreatePowerfullCodeDecreaser61iDecoder();
  Decoder::Ptr CreatePowerfullCodeDecreaser62Decoder();
  Decoder::Ptr CreateTRUSHDecoder();
  Decoder::Ptr CreateZXZipDecoder();
  Decoder::Ptr CreateZipDecoder();
  Decoder::Ptr CreateRarDecoder();
  Decoder::Ptr CreateGamePackerDecoder();
  Decoder::Ptr CreateGamePackerPlusDecoder();
  Decoder::Ptr CreateTurboLZDecoder();
  Decoder::Ptr CreateTurboLZProtectedDecoder();
  Decoder::Ptr CreateCharPresDecoder();
  Decoder::Ptr CreatePack2Decoder();
  Decoder::Ptr CreateLZH1Decoder();
  Decoder::Ptr CreateLZH2Decoder();
  Decoder::Ptr CreateFullDiskImageDecoder();
  Decoder::Ptr CreateHobetaDecoder();
  Decoder::Ptr CreateSna128Decoder();
  Decoder::Ptr CreateTeleDiskImageDecoder();
  Decoder::Ptr CreateCompiledASC0Decoder();
  Decoder::Ptr CreateCompiledASC1Decoder();
  Decoder::Ptr CreateCompiledASC2Decoder();
  Decoder::Ptr CreateCompiledST3Decoder();
  Decoder::Ptr CreateCompiledSTP1Decoder();
  Decoder::Ptr CreateCompiledSTP2Decoder();
  Decoder::Ptr CreateCompiledPT24Decoder();
  Decoder::Ptr CreateCompiledPTU13Decoder();
  Decoder::Ptr CreateZ80V145Decoder();
  Decoder::Ptr CreateZ80V20Decoder();
  Decoder::Ptr CreateZ80V30Decoder();
  Decoder::Ptr CreateMegaLZDecoder();
  Decoder::Ptr CreateDSKDecoder();
  Decoder::Ptr CreateGzipDecoder();
  Decoder::Ptr CreateMUSEDecoder();
}  // namespace Formats::Packed
