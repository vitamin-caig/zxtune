/*
Abstract:
  Hrust1-compatible bistream helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_ARCHIVES_PACK_HRUST1_BITSTREAM_H_DEFINED__
#define __CORE_PLUGINS_ARCHIVES_PACK_HRUST1_BITSTREAM_H_DEFINED__

//local includes
#include "pack_utils.h"

//Hrust1-compatible bitstream:
// -16 bit mask MSB->LSB order
// -mask is taken at the beginning
class Hrust1Bitstream : public ByteStream
{
public:
  Hrust1Bitstream(const uint8_t* data, std::size_t size)
    : ByteStream(data, size)
    , Bits(), Mask()
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

  uint_t GetBits(unsigned count)
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
  uint_t Bits;
  uint_t Mask;
};

#endif //__CORE_PLUGINS_ARCHIVES_PACK_HRUST1_BITSTREAM_H_DEFINED__
