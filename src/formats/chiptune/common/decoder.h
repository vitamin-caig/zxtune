/**
 *
 * @file
 *
 * @brief  Generic decoder for formats supported by format-matching description
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/decoder.h"

#include "string_view.h"

namespace Formats::Chiptune
{
  Decoder::Ptr CreateFormatDecoder(StringView format, StringView description);
}  // namespace Formats::Chiptune
