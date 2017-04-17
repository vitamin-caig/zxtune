/**
*
* @file
*
* @brief  Multitrack archives decoders factories
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <formats/archived.h>

namespace Formats
{
  namespace Archived
  {
    Decoder::Ptr CreateAYDecoder();
    Decoder::Ptr CreateSIDDecoder();

    //TODO: take Formats::Multitrack::Decoder::Ptr decoder as a parameter
    Decoder::Ptr CreateNSFDecoder();
    Decoder::Ptr CreateNSFEDecoder();
    Decoder::Ptr CreateGBSDecoder();
    Decoder::Ptr CreateSAPDecoder();
    Decoder::Ptr CreateKSSXDecoder();
    Decoder::Ptr CreateHESDecoder();
  }
}
