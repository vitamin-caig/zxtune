/**
 *
 * @file
 *
 * @brief  Packed-related utilities
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/data_builder.h"
#include "binary/view.h"

#include <algorithm>

namespace Formats::Packed
{
  class ByteStream
  {
  public:
    explicit ByteStream(Binary::View data)
      : Data(data.As<uint8_t>())
      , End(Data + data.Size())
      , Size(data.Size())
    {}

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

    std::size_t GetRestBytes() const
    {
      return End - Data;
    }

    std::size_t GetProcessedBytes() const
    {
      return Size - GetRestBytes();
    }

  private:
    const uint8_t* Data;
    const uint8_t* const End;
    const std::size_t Size;
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

  inline void Reverse(Binary::DataBuilder& data)
  {
    auto* start = &data.Get<uint8_t>(0);
    auto* end = start + data.Size();
    std::reverse(start, end);
  }

  inline void Fill(Binary::DataBuilder& data, std::size_t count, uint8_t byte)
  {
    auto* dst = static_cast<uint8_t*>(data.Allocate(count));
    std::fill_n(dst, count, byte);
  }

  template<class T>
  inline void Generate(Binary::DataBuilder& data, std::size_t count, T generator)
  {
    auto* dst = static_cast<uint8_t*>(data.Allocate(count));
    std::generate_n(dst, count, std::move(generator));
  }

  // offset to back
  inline bool CopyFromBack(std::size_t offset, Binary::DataBuilder& dst, std::size_t count)
  {
    if (offset > dst.Size())
    {
      return false;  // invalid backref
    }
    auto* dstStart = static_cast<uint8_t*>(dst.Allocate(count));
    const auto* srcStart = dstStart - offset;
    RecursiveCopy(srcStart, srcStart + count, dstStart);
    return true;
  }
}  // namespace Formats::Packed
