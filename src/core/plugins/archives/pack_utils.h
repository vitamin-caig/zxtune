/*
Abstract:
  Different packing-related utils

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_ARCHIVES_PACK_UTILS_H_DEFINED__
#define __CORE_PLUGINS_ARCHIVES_PACK_UTILS_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <algorithm>

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
