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

// common includes
#include <types.h>
// library includes
#include <binary/data_builder.h>
#include <binary/dump.h>
// std includes
#include <algorithm>
#include <cassert>
#include <cstring>

class ByteStream
{
public:
  ByteStream(const uint8_t* data, std::size_t size)
    : Data(data)
    , End(Data + size)
    , Size(size)
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

// src - first or last byte of source data to copy (e.g. hl)
// dst - first or last byte of target data to copy (e.g. de)
// count - count of bytes to copy (e.g. bc)
// dirCode - second byte of ldir/lddr operation (0xb8 for lddr, 0xb0 for ldir)
class DataMovementChecker
{
public:
  DataMovementChecker(uint_t src, uint_t dst, uint_t count, uint_t dirCode)
    : Forward(0xb0 == dirCode)
    , Backward(0xb8 == dirCode)
    , Source(src)
    , Target(dst)
    , Size(count)
  {
    (void)Source;  // to make compiler happy
  }

  bool IsValid() const
  {
    return Size && (Forward || Backward);
    // do not check other due to overlap possibility
  }

  uint_t FirstOfMovedData() const
  {
    assert(Forward != Backward);
    return Forward ? Target : (Target - Size + 1) & 0xffff;
  }

  uint_t LastOfMovedData() const
  {
    assert(Forward != Backward);
    return Backward ? Target : (Target + Size - 1) & 0xffff;
  }

private:
  const bool Forward;
  const bool Backward;
  const uint_t Source;
  const uint_t Target;
  const uint_t Size;
};

inline std::size_t MatchedSize(const uint8_t* dataBegin, const uint8_t* dataEnd, const uint8_t* patBegin,
                               const uint8_t* patEnd)
{
  const std::size_t dataSize = dataEnd - dataBegin;
  const std::size_t patSize = patEnd - patBegin;
  if (dataSize >= patSize)
  {
    const std::pair<const uint8_t*, const uint8_t*> mismatch = std::mismatch(patBegin, patEnd, dataBegin);
    return mismatch.first - patBegin;
  }
  else
  {
    const std::pair<const uint8_t*, const uint8_t*> mismatch = std::mismatch(dataBegin, dataEnd, patBegin);
    return mismatch.first - dataBegin;
  }
}
