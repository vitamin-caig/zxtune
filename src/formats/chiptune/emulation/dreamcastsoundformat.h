/**
 *
 * @file
 *
 * @brief  DSF parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace DreamcastSoundFormat
  {
    const uint_t VERSION_ID = 0x12;
  }

  Decoder::Ptr CreateDSFDecoder();
}  // namespace Formats::Chiptune
