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
#include <bit>
#include <types.h>
// std includes
#include <cstring>

//! @brief Checking if current platform is Little-Endian
constexpr bool isLE()
{
  static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big,
                "Invalid byte order");
  return std::endian::native == std::endian::little;
}

// TODO: remove at std++23
constexpr uint16_t swapBytes(uint16_t a)
{
  return uint16_t(a << 8) | uint16_t(a >> 8);
}

constexpr uint32_t swapBytes(uint32_t a)
{
  const uint32_t tmp((((a >> 8) ^ (a << 8)) & 0xff00ff) ^ (a << 8));
  return (tmp << 16) | (tmp >> 16);
}

constexpr uint64_t swapBytes(uint64_t a)
{
  const uint64_t a1 = ((a & 0x00ff00ff00ff00ffuLL) << 8) | ((a & 0xff00ff00ff00ff00uLL) >> 8);
  const uint64_t a2 = ((a1 & 0x0000ffff0000ffffuLL) << 16) | ((a1 & 0xffff0000ffff0000uLL) >> 16);
  return (a2 << 32) | (a2 >> 32);
}

template<class T, std::enable_if_t<std::is_signed_v<T>, bool> = true>
constexpr T swapBytes(T a)
{
  using Base = std::make_unsigned_t<T>;
  return static_cast<T>(swapBytes(static_cast<Base>(a)));
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

  template<class T, std::size_t Size>
  constexpr inline T ReadRaw(const void* data)
  {
    T ret{};
    std::memcpy(&ret, data, Size);
    return ret;
  }

  template<class T, std::size_t Size>
  inline void WriteRaw(T val, void* data)
  {
    std::memcpy(data, &val, Size);
  }
}  // namespace Details

template<class T, std::size_t Size = sizeof(T)>
constexpr inline T ReadLE(const void* data)
{
  if constexpr (isLE())
  {
    return Details::ReadRaw<T, Size>(data);
  }
  else
  {
    using Base = typename std::make_unsigned<T>::type;
    return static_cast<T>(Details::Endian<Base, Size>::ReadLE(static_cast<const uint8_t*>(data)));
  }
}

template<class T, std::size_t Size = sizeof(T)>
constexpr inline T ReadBE(const void* data)
{
  if constexpr (isLE())
  {
    using Base = typename std::make_unsigned<T>::type;
    return static_cast<T>(Details::Endian<Base, Size>::ReadBE(static_cast<const uint8_t*>(data)));
  }
  else
  {
    // zero high bytes
    return Details::ReadRaw<T, Size>(data) >> 8 * (sizeof(T) - Size);
  }
}

template<class T, std::size_t Size = sizeof(T)>
inline void WriteLE(T val, void* data)
{
  if constexpr (isLE())
  {
    Details::WriteRaw<T, Size>(val, data);
  }
  else
  {
    using Base = typename std::make_unsigned<T>::type;
    Details::Endian<Base, Size>::WriteLE(static_cast<Base>(val), static_cast<uint8_t*>(data));
  }
}

template<class T, std::size_t Size = sizeof(T)>
inline void WriteBE(T val, void* data)
{
  if constexpr (isLE())
  {
    using Base = typename std::make_unsigned<T>::type;
    Details::Endian<Base, Size>::WriteBE(static_cast<Base>(val), static_cast<uint8_t*>(data));
  }
  else
  {
    // drop high bytes
    Details::WriteRaw<T, Size>(val << 8 * (sizeof(T) - Size), data);
  }
}

template<class T, std::size_t Size = sizeof(T)>
class BE
{
public:
  BE() = default;

  BE(T val)
  {
    WriteBE<T, Size>(val, Storage);
  }

  constexpr operator T() const
  {
    return ReadBE<T, Size>(Storage);
  }

  BE& operator=(T val)
  {
    WriteBE<T, Size>(val, Storage);
    return *this;
  }

private:
  uint8_t Storage[Size];
};

template<class T, std::size_t Size = sizeof(T)>
class LE
{
public:
  LE() = default;

  LE(T val)
  {
    WriteLE<T, Size>(val, Storage);
  }

  constexpr operator T() const
  {
    return ReadLE<T, Size>(Storage);
  }

  LE& operator=(T val)
  {
    WriteLE<T, Size>(val, Storage);
    return *this;
  }

private:
  uint8_t Storage[Size];
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

// additional
using le_uint24_t = LE<uint32_t, 3>;
using be_uint24_t = BE<uint32_t, 3>;
