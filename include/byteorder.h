/*
Abstract:
  Helper functions for byteorder working

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __BYTEORDER_H_DEFINED__
#define __BYTEORDER_H_DEFINED__

#include "types.h"
#include <boost/detail/endian.hpp>

//byte order macroses
inline uint16_t swapBytes(uint16_t a)
{
  return (a << 8) | (a >> 8);
}

inline uint32_t swapBytes(uint32_t a)
{
  const uint32_t tmp((((a >> 8) ^ (a << 8)) & 0xff00ff) ^ (a << 8));
  return (tmp << 16) | (tmp >> 16);
}

inline uint64_t swapBytes(uint64_t a)
{
  const uint64_t a1 = ((a & UINT64_C(0x00ff00ff00ff00ff)) << 8) |
                      ((a & UINT64_C(0xff00ff00ff00ff00)) >> 8);
  const uint64_t a2 = ((a1 & UINT64_C(0x0000ffff0000ffff)) << 16) |
                      ((a1 & UINT64_C(0xffff0000ffff0000)) >> 16);
  return (a2 << 32) | (a2 >> 32);
}

#ifdef BOOST_LITTLE_ENDIAN
inline bool isLE()
{
  return true;
}

template<class T>
inline T fromLE(T a)
{
  return a;
}

template<class T>
inline T fromBE(T a)
{
  return swapBytes(a);
}

#elif defined(BOOST_BIG_ENDIAN)
inline bool isLE()
{
  return false;
}

template<class T>
inline T fromLE(T a)
{
  return swapBytes(a);
}

template<class T>
inline T fromBE(T a)
{
  return a;
}
#else
#error Invalid byte order
#endif

#endif //__BYTEORDER_H_DEFINED__
