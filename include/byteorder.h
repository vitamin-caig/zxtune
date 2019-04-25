/**
*
* @file
*
* @brief  Helper functions for byteorder working
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//boost includes
#include <boost/predef/other/endian.h>

template<std::size_t size>
struct ByteSwap;

template<>
struct ByteSwap<2>
{
  typedef uint16_t Type;
  inline static Type Swap(Type a)
  {
    return (a << 8) | (a >> 8);
  }
};

template<>
struct ByteSwap<4>
{
  typedef uint32_t Type;
  inline static Type Swap(Type a)
  {
    const Type tmp((((a >> 8) ^ (a << 8)) & 0xff00ff) ^ (a << 8));
    return (tmp << 16) | (tmp >> 16);
  }
};

template<>
struct ByteSwap<8>
{
  typedef uint64_t Type;
  inline static Type Swap(Type a)
  {
    const Type a1 = ((a & UINT64_C(0x00ff00ff00ff00ff)) << 8) |
                    ((a & UINT64_C(0xff00ff00ff00ff00)) >> 8);
    const Type a2 = ((a1 & UINT64_C(0x0000ffff0000ffff)) << 16) |
                    ((a1 & UINT64_C(0xffff0000ffff0000)) >> 16);
    return (a2 << 32) | (a2 >> 32);
  }
};

//! @brief Swapping byteorder of integer type value
template<class T>
inline T swapBytes(T a)
{
  typedef ByteSwap<sizeof(T)> Swapper;
  return static_cast<T>(Swapper::Swap(static_cast<typename Swapper::Type>(a)));
}

#ifdef BOOST_ENDIAN_LITTLE_BYTE
//! @brief Checking if current platform is Little-Endian
inline bool isLE()
{
  return true;
}

//! @brief Converting input data from Little-Endian byteorder
template<class T>
inline T fromLE(T a)
{
  return a;
}

//! @brief Converting input data from Big-Endian byteorder
template<class T>
inline T fromBE(T a)
{
  return swapBytes(a);
}

#elif defined(BOOST_ENDIAN_BIG_BYTE)
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
