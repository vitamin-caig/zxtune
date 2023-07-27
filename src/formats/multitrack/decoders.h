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

#include "formats/multitrack.h"

namespace Formats::Multitrack
{
  Decoder::Ptr CreateSIDDecoder();
  Decoder::Ptr CreateNSFDecoder();
  Decoder::Ptr CreateNSFEDecoder();
  Decoder::Ptr CreateGBSDecoder();
  Decoder::Ptr CreateSAPDecoder();
  Decoder::Ptr CreateKSSXDecoder();
  Decoder::Ptr CreateHESDecoder();
  Decoder::Ptr CreateSNDHDecoder();
}  // namespace Formats::Multitrack
