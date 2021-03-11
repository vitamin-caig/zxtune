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

// common includes
#include <types.h>
// boost includes
#include <boost/predef/other/endian.h>

template<std::size_t size>
struct Byteorder;

template<>
struct Byteorder<2>
{
  typedef uint16_t Type;

  inline static Type Swap(Type a)
  {
    return (a << 8) | (a >> 8);
  }

  inline static Type ReadLE(const uint8_t* in)
  {
    return (Type(in[1]) << 8) | in[0];
  }

  inline static Type ReadBE(const uint8_t* in)
  {
    return (Type(in[0]) << 8) | in[1];
  }
};

// Artificial
template<>
struct Byteorder<3>
{
  typedef uint_t Type;

  inline static Type ReadLE(const uint8_t* in)
  {
    return (Type(in[2]) << 16) | (Type(in[1]) << 8) | in[0];
  }

  inline static Type ReadBE(const uint8_t* in)
  {
    return (Type(in[0]) << 16) | (Type(in[1]) << 8) | in[2];
  }
};

template<>
struct Byteorder<4>
{
  typedef uint32_t Type;

  inline static Type Swap(Type a)
  {
    const Type tmp((((a >> 8) ^ (a << 8)) & 0xff00ff) ^ (a << 8));
    return (tmp << 16) | (tmp >> 16);
  }

  inline static Type ReadLE(const uint8_t* in)
  {
    return (Type(in[3]) << 24) | (Type(in[2]) << 16) | (Type(in[1]) << 8) | in[0];
  }

  inline static Type ReadBE(const uint8_t* in)
  {
    return (Type(in[0]) << 24) | (Type(in[1]) << 16) | (Type(in[2]) << 8) | in[3];
  }
};

template<>
struct Byteorder<8>
{
  typedef uint64_t Type;

  inline static Type Swap(Type a)
  {
    const Type a1 = ((a & UINT64_C(0x00ff00ff00ff00ff)) << 8) | ((a & UINT64_C(0xff00ff00ff00ff00)) >> 8);
    const Type a2 = ((a1 & UINT64_C(0x0000ffff0000ffff)) << 16) | ((a1 & UINT64_C(0xffff0000ffff0000)) >> 16);
    return (a2 << 32) | (a2 >> 32);
  }

  inline static Type ReadLE(const uint8_t* in)
  {
    return (Type(in[7]) << 56) | (Type(in[6]) << 48) | (Type(in[5]) << 40) | (Type(in[4]) << 32) | (Type(in[3]) << 24)
           | (Type(in[2]) << 16) | (Type(in[1]) << 8) | in[0];
  }

  inline static Type ReadBE(const uint8_t* in)
  {
    return (Type(in[0]) << 56) | (Type(in[1]) << 48) | (Type(in[2]) << 40) | (Type(in[3]) << 32) | (Type(in[4]) << 24)
           | (Type(in[5]) << 16) | (Type(in[6]) << 8) | in[7];
  }
};

//! @brief Swapping byteorder of integer type value
template<class T>
inline T swapBytes(T a)
{
  typedef Byteorder<sizeof(T)> Bytes;
  return static_cast<T>(Bytes::Swap(static_cast<typename Bytes::Type>(a)));
}

template<class T>
inline T ReadLE(const uint8_t* in)
{
  typedef Byteorder<sizeof(T)> Bytes;
  return static_cast<T>(Bytes::ReadLE(in));
}

template<class T>
inline T ReadBE(const uint8_t* in)
{
  typedef Byteorder<sizeof(T)> Bytes;
  return static_cast<T>(Bytes::ReadBE(in));
}

#if BOOST_ENDIAN_LITTLE_BYTE
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

#elif BOOST_ENDIAN_BIG_BYTE
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
#  error Invalid byte order
#endif
