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

// library includes
#include <formats/multitrack.h>

namespace Formats::Multitrack
{
  Decoder::Ptr CreateSIDDecoder();
  Decoder::Ptr CreateNSFDecoder();
  Decoder::Ptr CreateNSFEDecoder();
  Decoder::Ptr CreateGBSDecoder();
  Decoder::Ptr CreateSAPDecoder();
  Decoder::Ptr CreateKSSXDecoder();
  Decoder::Ptr CreateHESDecoder();
}  // namespace Formats::Multitrack
