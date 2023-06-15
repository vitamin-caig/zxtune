/**
 *
 * @file
 *
 * @brief  USF parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <binary/view.h>
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace Ultra64SoundFormat
  {
    const uint_t VERSION_ID = 0x21;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual void SetRom(uint32_t offset, Binary::View content) = 0;
      virtual void SetSaveState(uint32_t offset, Binary::View content) = 0;
    };

    void ParseSection(Binary::View data, Builder& target);
  }  // namespace Ultra64SoundFormat

  Decoder::Ptr CreateUSFDecoder();
}  // namespace Formats::Chiptune
