/**
 *
 * @file
 *
 * @brief  Hrust v1.x- compatible bitstream helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/packed/common/utils.h"

namespace Formats::Packed::Hrust1
{
  // Hrust1-compatible bitstream:
  // -16 bit mask MSB->LSB order
  // -mask is taken at the beginning
  class Bitstream : public ByteStream
  {
  public:
    explicit Bitstream(Binary::View data)
      : ByteStream(data)
    {
      InitMask();
    }

    uint_t GetBit()
    {
      const uint_t result = (Bits & Mask) != 0 ? 1 : 0;
      if (!(Mask >>= 1))
      {
        InitMask();
      }
      return result;
    }

    uint_t GetBits(uint_t count)
    {
      uint_t result = 0;
      while (count--)
      {
        result = 2 * result | GetBit();
      }
      return result;
    }

    uint_t GetLen()
    {
      uint_t len = 0;
      for (uint_t bits = 3; bits == 0x3 && len != 0xf;)
      {
        bits = GetBits(2);
        len += bits;
      }
      return len;
    }

  private:
    void InitMask()
    {
      Bits = GetLEWord();
      Mask = 0x8000;
    }

  private:
    uint_t Bits = 0;
    uint_t Mask = 0;
  };
}  // namespace Formats::Packed::Hrust1