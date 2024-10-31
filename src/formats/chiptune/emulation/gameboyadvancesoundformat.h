/**
 *
 * @file
 *
 * @brief  GSF parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <binary/view.h>
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace GameBoyAdvanceSoundFormat
  {
    const uint_t VERSION_ID = 0x22;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual void SetEntryPoint(uint32_t addr) = 0;
      virtual void SetRom(uint32_t addr, Binary::View content) = 0;
    };

    void ParseRom(Binary::View data, Builder& target);
  }  // namespace GameBoyAdvanceSoundFormat

  Decoder::Ptr CreateGSFDecoder();
}  // namespace Formats::Chiptune
