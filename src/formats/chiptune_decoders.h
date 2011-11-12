/**
*
* @file     formats/chiptune_decoders.h
* @brief    Chiptune analyzers factories
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __FORMATS_CHIPTUNE_DECODERS_H_DEFINED__
#define __FORMATS_CHIPTUNE_DECODERS_H_DEFINED__

//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    Decoder::Ptr CreatePSGDecoder();
    Decoder::Ptr CreateDigitalStudioDecoder();
  }
}

#endif //__FORMATS_CHIPTUNE_DECODERS_H_DEFINED__
