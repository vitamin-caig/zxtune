/*
Abstract:
  Hrust1-compatible bistream helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_ARCHIVES_PACK_HRUST1_BITSTREAM_H_DEFINED__
#define __CORE_PLUGINS_ARCHIVES_PACK_HRUST1_BITSTREAM_H_DEFINED__

//common includes
#include <types.h>

//Hrust1-compatible bitstream:
// -16 bit mask MSB->LSB order
// -mask is taken at the beginning
class Hrust1Bitstream
{
public:
  Hrust1Bitstream(const uint8_t* data, std::size_t size)
    : Data(data), End(Data + size), Bits(), Mask(0x8000)
  {
    Bits = GetByte();
    Bits |= 256 * GetByte();
  }

  bool Eof() const
  {
    return Data >= End;
  }

  uint8_t GetByte()
  {
    return Eof() ? 0 : *Data++;
  }

  uint_t GetBit()
  {
    const uint_t result = (Bits & Mask) != 0 ? 1 : 0;
    if (!(Mask >>= 1))
    {
      Bits = GetByte();
      Bits |= 256 * GetByte();
      Mask = 0x8000;
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
private:
  const uint8_t* Data;
  const uint8_t* const End;
  uint_t Bits;
  uint_t Mask;
};

#endif //__CORE_PLUGINS_ARCHIVES_PACK_HRUST1_BITSTREAM_H_DEFINED__
