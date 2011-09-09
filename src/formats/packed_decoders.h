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
    Decoder::Ptr CreateHrust2Decoder();
    Decoder::Ptr CreateHrust2RawDecoder();
    Decoder::Ptr CreateLZSDecoder();
    Decoder::Ptr CreateMSPackDecoder();
    Decoder::Ptr CreatePowerfullCodeDecreaser61Decoder();
    Decoder::Ptr CreatePowerfullCodeDecreaser62Decoder();
    Decoder::Ptr CreateTRUSHDecoder();
    Decoder::Ptr CreateZXZipDecoder();
    Decoder::Ptr CreateZipDecoder();
  }
}

#endif //__FORMATS_PACKED_DECODERS_H_DEFINED__
