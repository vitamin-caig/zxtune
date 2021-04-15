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

//! @brief Checking if current platform is Little-Endian
constexpr bool isLE()
{
#if BOOST_ENDIAN_LITTLE_BYTE
  return true;
#elif BOOST_ENDIAN_BIG_BYTE
  return false
#else
#  error Invalid byte order
#endif
}

template<std::size_t size>
struct Byteorder;

template<>
struct Byteorder<2>
{
  typedef uint16_t Type;

  constexpr static Type Swap(Type a)
  {
    return Type(a << 8) | Type(a >> 8);
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

  constexpr static Type Swap(Type a)
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

  constexpr static Type Swap(Type a)
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
constexpr T swapBytes(T a)
{
  typedef Byteorder<sizeof(T)> Bytes;
  return static_cast<T>(Bytes::Swap(static_cast<typename Bytes::Type>(a)));
}

namespace Details
{
  template<class T, int Rest, int Done = 0>
  struct Endian
  {
    static_assert(std::is_unsigned<T>::value, "Only unsigned types allowed");

    constexpr static T ReadLE(const uint8_t* data)
    {
      return (T(data[Done]) << (8 * Done)) | Endian<T, Rest - 1, Done + 1>::ReadLE(data);
    }

    constexpr static T ReadBE(const uint8_t* data)
    {
      return (T(data[Rest - 1]) << (8 * Done)) | Endian<T, Rest - 1, Done + 1>::ReadBE(data);
    }

    static void WriteLE(T val, uint8_t* data)
    {
      data[Done] = (val >> (8 * Done)) & 0xff;
      Endian<T, Rest - 1, Done + 1>::WriteLE(val, data);
    }

    static void WriteBE(T val, uint8_t* data)
    {
      data[Rest - 1] = (val >> (8 * Done)) & 0xff;
      Endian<T, Rest - 1, Done + 1>::WriteBE(val, data);
    }
  };

  template<class T, int Done>
  struct Endian<T, 0, Done>
  {
    constexpr static T ReadLE(const uint8_t*)
    {
      return 0;
    }

    constexpr static T ReadBE(const uint8_t*)
    {
      return 0;
    }

    static void WriteLE(T, uint8_t*) {}

    static void WriteBE(T, uint8_t*) {}
  };

  template<class T>
  constexpr inline T ReadRaw(const void* data)
  {
    T ret{};
    std::memcpy(&ret, data, sizeof(ret));
    return ret;
  }

  template<class T>
  inline void WriteRaw(T val, void* data)
  {
    std::memcpy(data, &val, sizeof(val));
  }
}  // namespace Details

template<class T>
constexpr inline T ReadLE(const void* data)
{
  if constexpr (isLE())
  {
    return Details::ReadRaw<T>(data);
  }
  else
  {
    using Base = typename std::make_unsigned<T>::type;
    return static_cast<T>(Details::Endian<Base, sizeof(Base)>::ReadLE(static_cast<const uint8_t*>(data)));
  }
}

template<class T>
constexpr inline T ReadBE(const void* data)
{
  if constexpr (isLE())
  {
    using Base = typename std::make_unsigned<T>::type;
    return static_cast<T>(Details::Endian<Base, sizeof(Base)>::ReadBE(static_cast<const uint8_t*>(data)));
  }
  else
  {
    return Details::ReadRaw<T>(data);
  }
}

template<class T>
inline void WriteLE(T val, void* data)
{
  if constexpr (isLE())
  {
    Details::WriteRaw(val, data);
  }
  else
  {
    using Base = typename std::make_unsigned<T>::type;
    Details::Endian<Base, sizeof(Base)>::WriteLE(static_cast<Base>(val), static_cast<uint8_t*>(data));
  }
}

template<class T>
inline void WriteBE(T val, void* data)
{
  if constexpr (isLE())
  {
    using Base = typename std::make_unsigned<T>::type;
    Details::Endian<Base, sizeof(Base)>::WriteBE(static_cast<Base>(val), static_cast<uint8_t*>(data));
  }
  else
  {
    Details::WriteRaw(val, data);
  }
}

template<class T>
class BE
{
public:
  BE() = default;

  BE(T val)
  {
    WriteBE(val, Storage);
  }

  constexpr operator T() const
  {
    return ReadBE<T>(Storage);
  }

  BE& operator=(T val)
  {
    WriteBE(val, Storage);
    return *this;
  }

private:
  uint8_t Storage[sizeof(T)];
};

template<class T>
class LE
{
public:
  LE() = default;

  LE(T val)
  {
    WriteLE(val, Storage);
  }

  constexpr operator T() const
  {
    return ReadLE<T>(Storage);
  }

  LE& operator=(T val)
  {
    WriteLE(val, Storage);
    return *this;
  }

private:
  uint8_t Storage[sizeof(T)];
};

using le_uint16_t = LE<uint16_t>;
using le_uint32_t = LE<uint32_t>;
using le_uint64_t = LE<uint64_t>;
using le_int16_t = LE<int16_t>;
using le_int32_t = LE<int32_t>;
using le_int64_t = LE<int64_t>;
using be_uint16_t = BE<uint16_t>;
using be_uint32_t = BE<uint32_t>;
using be_uint64_t = BE<uint64_t>;
using be_int16_t = BE<int16_t>;
using be_int32_t = BE<int32_t>;
using be_int64_t = BE<int64_t>;

//! @brief Converting input data from Little-Endian byteorder
template<typename T>
inline T fromLE(T a)
{
  static_assert(std::is_integral<T>::value, "Not an integral");
  if constexpr (isLE())
  {
    return a;
  }
  else
  {
    return swapBytes(a);
  }
}

//! @brief Converting input data from Big-Endian byteorder
template<typename T>
inline T fromBE(T a)
{
  static_assert(std::is_integral<T>::value, "Not an integral");
  if constexpr (isLE())
  {
    return swapBytes(a);
  }
  else
  {
    return a;
  }
}
