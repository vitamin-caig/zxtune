/**
 *
 * @file
 *
 * @brief  NCSF parser interface
 *
 * @author liushuyu011@gmail.com
 *
 **/

#pragma once

#include "binary/view.h"
#include "formats/chiptune.h"

namespace Formats::Chiptune
{
  namespace NitroComposerSoundFormat
  {
    const uint_t VERSION_ID = 0x25;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual void SetChunk(uint32_t offset, Binary::View content) = 0;
    };

    // pass `data` to the function, and `target` will contain the program data (SDAT) from the input
    void ParseRom(Binary::View data, Builder& target);
    // since ncsf file does not actually have a state,
    // the "state" returned here is the sequence number of the track to be played (sseq)
    uint32_t ParseState(Binary::View data);
  }  // namespace NitroComposerSoundFormat

  Decoder::Ptr CreateNCSFDecoder();
}  // namespace Formats::Chiptune
