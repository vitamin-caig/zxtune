/**
*
* @file     formats/image/decoders.h
* @brief    Image data accessors factories
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef FORMATS_IMAGE_DECODERS_H_DEFINED
#define FORMATS_IMAGE_DECODERS_H_DEFINED

//library includes
#include <formats/packed.h>

namespace Formats
{
  namespace Image
  {
    Decoder::Ptr CreateLaserCompact52Decoder();
    Decoder::Ptr CreateASCScreenCrusherDecoder();
  }
}

#endif //FORMATS_IMAGE_DECODERS_H_DEFINED
