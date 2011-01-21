/*
Abstract:
  Different packing-related utils

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_ARCHIVES_PACK_UTILS_H_DEFINED__
#define __CORE_PLUGINS_ARCHIVES_PACK_UTILS_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <algorithm>

class ByteStream
{
public:
  ByteStream(const uint8_t* data, std::size_t size)
    : Data(data), End(Data + size)
  {
  }

  bool Eof() const
  {
    return Data >= End;
  }

  uint8_t GetByte()
  {
    return Eof() ? 0 : *Data++;
  }

  uint_t GetLEWord()
  {
    const uint_t lo = GetByte();
    const uint_t hi = GetByte();
    return 256 * hi + lo;
  }
private:
  const uint8_t* Data;
  const uint8_t* const End;
};

template<class Iterator, class ConstIterator>
void RecursiveCopy(ConstIterator srcBegin, ConstIterator srcEnd, Iterator dstBegin)
{
  const ConstIterator constDst = dstBegin;
  if (std::distance(srcEnd, constDst) >= 0)
  {
    std::copy(srcBegin, srcEnd, dstBegin);
  }
  else
  {
    Iterator dst = dstBegin;
    for (ConstIterator src = srcBegin; src != srcEnd; ++src, ++dst)
    {
      *dst = *src;
    }
  }
}

//offset to back
inline bool CopyFromBack(uint_t offset, Dump& dst, uint_t count)
{
  const std::size_t size = dst.size();
  if (offset > size)
  {
    return false;//invalid backref
  }
  dst.resize(size + count);
  const Dump::iterator dstStart = dst.begin() + size;
  const Dump::const_iterator srcStart = dstStart - offset;
  const Dump::const_iterator srcEnd = srcStart + count;
  RecursiveCopy(srcStart, srcEnd, dstStart);
  return true;
}

#endif //__CORE_PLUGINS_ARCHIVES_PACK_UTILS_H_DEFINED__
