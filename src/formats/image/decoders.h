/**
 *
 * @file
 *
 * @brief  Image decoders factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/image/decoder.h"

namespace Formats::Image
{
  Decoder::Ptr CreateLaserCompact52Decoder();
  Decoder::Ptr CreateASCScreenCrusherDecoder();
  Decoder::Ptr CreateLaserCompact40Decoder();
}  // namespace Formats::Image
