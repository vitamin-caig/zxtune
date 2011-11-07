/**
*
* @file     formats/packed_decoders.h
* @brief    Packed data accessors factories
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __FORMATS_PACKED_DECODERS_H_DEFINED__
#define __FORMATS_PACKED_DECODERS_H_DEFINED__

//library includes
#include <formats/packed.h>

namespace Formats
{
  namespace Packed
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
  }
}

#endif //__FORMATS_PACKED_DECODERS_H_DEFINED__
