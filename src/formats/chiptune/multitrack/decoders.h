/**
*
* @file
*
* @brief  Multitrack chiptunes decoders factories
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <formats/chiptune.h>
#include <formats/multitrack.h>

namespace Formats
{
  namespace Chiptune
  {
    Decoder::Ptr CreateNSFDecoder(Formats::Multitrack::Decoder::Ptr decoder);
    Decoder::Ptr CreateNSFEDecoder(Formats::Multitrack::Decoder::Ptr decoder);
    Decoder::Ptr CreateGBSDecoder(Formats::Multitrack::Decoder::Ptr decoder);
  }
}
