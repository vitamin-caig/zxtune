/**
 *
 * @file
 *
 * @brief  Packed-related utilities for ZX
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/view.h"

#include <algorithm>
#include <cassert>

namespace Formats::Packed
{
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

  inline bool CheckDataMovement(uint_t src, uint_t dst, uint_t count, uint_t dirCode)
  {
    return DataMovementChecker(src, dst, count, dirCode).IsValid();
  }

  inline std::size_t MatchedSize(Binary::View data, Binary::View pattern)
  {
    const auto* dataBegin = data.As<uint8_t>();
    const auto* patBegin = pattern.As<uint8_t>();
    const auto size = std::min(data.Size(), pattern.Size());
    return std::mismatch(dataBegin, dataBegin + size, patBegin).first - dataBegin;
  }
}  // namespace Formats::Packed
