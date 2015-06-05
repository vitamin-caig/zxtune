/**
* 
* @file
*
* @brief  Multitrack decoders factories
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <formats/multitrack.h>

namespace Formats
{
  namespace Multitrack
  {
    Decoder::Ptr CreateNSFDecoder();
    Decoder::Ptr CreateNSFEDecoder();
    Decoder::Ptr CreateGBSDecoder();
    Decoder::Ptr CreateSAPDecoder();
  }
}
