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

// library includes
#include <formats/image.h>

namespace Formats::Image
{
  Decoder::Ptr CreateLaserCompact52Decoder();
  Decoder::Ptr CreateASCScreenCrusherDecoder();
  Decoder::Ptr CreateLaserCompact40Decoder();
}  // namespace Formats::Image
