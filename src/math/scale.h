/**
 *
 * @file
 *
 * @brief  Scaling functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <math/bitops.h>
// std includes
#include <cassert>

namespace Math
{
  //! @brief Scales value in range of inRange to value in outRange
  inline constexpr uint8_t Scale(uint8_t value, uint8_t inRange, uint8_t outRange)
  {
    return static_cast<uint8_t>(uint16_t(value) * outRange / inRange);
  }

  inline constexpr uint16_t Scale(uint16_t value, uint16_t inRange, uint16_t outRange)
  {
    return static_cast<uint16_t>(uint32_t(value) * outRange / inRange);
  }

  inline constexpr uint32_t Scale(uint32_t value, uint32_t inRange, uint32_t outRange)
  {
    return static_cast<uint32_t>(uint64_t(value) * outRange / inRange);
  }

  template<class T1, class T2>
  inline constexpr std::pair<T1, T2> OptimizeRatio(T1 first, T2 second)
  {
    return 0 == ((first | second) & 1) ? OptimizeRatio<T1, T2>(first / 2, second / 2) : std::make_pair(first, second);
  }

  inline constexpr uint64_t Scale(uint64_t value, uint64_t inRange, uint64_t outRange)
  {
    // really reverseBits(significantBits(val))
    const auto unsafeScaleMask = HiBitsMask<uint64_t>(Log2(outRange));
    if (0 == (value & unsafeScaleMask))
    {
      return (value * outRange) / inRange;
    }
    else
    {
      const std::pair<uint64_t, uint64_t> valIn = OptimizeRatio(value, inRange);
      const std::pair<uint64_t, uint64_t> outIn = OptimizeRatio(outRange, valIn.second);
      return outIn.second != inRange ? Scale(valIn.first, outIn.second, outIn.first)
                                     : static_cast<uint64_t>(double(value) * outRange / inRange);
    }
  }

  template<class T>
  class ScaleFunctor
  {
  public:
    constexpr ScaleFunctor(T inRange, T outRange)
      : InRange(inRange)
      , OutRange(outRange)
    {}

    ScaleFunctor(const ScaleFunctor<T>& rh)
      : InRange(rh.InRange)
      , OutRange(rh.OutRange)
    {}

    constexpr T operator()(T value) const
    {
      return Scale(value, InRange, OutRange);
    }

  private:
    T InRange;
    T OutRange;
  };

  template<>
  class ScaleFunctor<uint64_t>
  {
  public:
    constexpr ScaleFunctor(uint64_t inRange, uint64_t outRange)
      : Range(OptimizeRatio(inRange, outRange))
      , UnsafeScaleMask(HiBitsMask<uint64_t>(Log2(Range.second)))
    {}

    ScaleFunctor(const ScaleFunctor<uint64_t>& rh) = default;

    constexpr uint64_t operator()(uint64_t value) const
    {
      if (0 == (value & UnsafeScaleMask))
      {
        return (value * Range.second) / Range.first;
      }
      else
      {
        return static_cast<uint64_t>(double(value) * Range.second / Range.first);
      }
    }

  private:
    std::pair<uint64_t, uint64_t> Range;
    uint64_t UnsafeScaleMask;
  };
}  // namespace Math
