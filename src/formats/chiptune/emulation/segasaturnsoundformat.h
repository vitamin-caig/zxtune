/**
 *
 * @file
 *
 * @brief  SSF parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace SegaSaturnSoundFormat
  {
    const uint_t VERSION_ID = 0x11;
  }

  Decoder::Ptr CreateSSFDecoder();
}  // namespace Formats::Chiptune
