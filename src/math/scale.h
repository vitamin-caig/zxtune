/**
*
* @file     scale.h
* @brief    Scaling functions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef MATH_SCALE_H_DEFINED
#define MATH_SCALE_H_DEFINED

//library includes
#include <math/bitops.h>

namespace Math
{
  //! @brief Scales value in range of inRange to value in outRange
  inline uint8_t Scale(uint8_t value, uint8_t inRange, uint8_t outRange)
  {
    return static_cast<uint8_t>(uint16_t(value) * outRange / inRange);
  }

  inline uint16_t Scale(uint16_t value, uint16_t inRange, uint16_t outRange)
  {
    return static_cast<uint16_t>(uint32_t(value) * outRange / inRange);
  }

  inline uint32_t Scale(uint32_t value, uint32_t inRange, uint32_t outRange)
  {
    return static_cast<uint32_t>(uint64_t(value) * outRange / inRange);
  }

  inline std::pair<uint64_t, uint64_t> OptimizeRatio(uint64_t first, uint64_t second)
  {
    while (0 == ((first | second) & 1))
    {
      first >>= 1;
      second >>= 1;
    }
    return std::make_pair(first, second);
  }

  inline uint64_t Scale(uint64_t value, uint64_t inRange, uint64_t outRange)
  {
    const std::size_t valBits = Log2(value);
    const std::size_t outBits = Log2(outRange);
    const std::size_t availBits = 8 * sizeof(uint64_t);
    if (valBits + outBits < availBits)
    {
      return (value * outRange) / inRange;
    }
    if (valBits + outBits - Log2(inRange) > availBits)
    {
      return ~uint64_t(0);//maximal value
    }
    const std::pair<uint64_t, uint64_t> valIn = OptimizeRatio(value, inRange);
    const std::pair<uint64_t, uint64_t> outIn = OptimizeRatio(outRange, valIn.second);
    return outIn.second != inRange
        ? Scale(valIn.first, outIn.second, outIn.first)
        : static_cast<uint64_t>(double(value) * outRange / inRange);
  }

  template<class T>
  class ScaleFunctor
  {
  public:
    ScaleFunctor(T inRange, T outRange)
      : InRange(inRange)
      , OutRange(outRange)
    {
    }

    ScaleFunctor(const ScaleFunctor<T>& rh)
      : InRange(rh.InRange)
      , OutRange(rh.OutRange)
    {
    }

    T operator()(T value) const
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
    ScaleFunctor(uint64_t inRange, uint64_t outRange)
      : Range(OptimizeRatio(inRange, outRange))
      , RangeBits(std::make_pair(Log2(Range.first), Log2(Range.second)))
    {
    }

    ScaleFunctor(const ScaleFunctor<uint64_t>& rh)
      : Range(rh.Range)
      , RangeBits(rh.RangeBits)
    {
    }

    uint64_t operator()(uint64_t value) const
    {
      const std::size_t valBits = Log2(value);
      const std::size_t availBits = 8 * sizeof(uint64_t);
      if (valBits + RangeBits.second < availBits)
      {
        return (value * Range.second) / Range.first;
      }
      if (valBits + RangeBits.second - RangeBits.first > availBits)
      {
        return ~uint64_t(0);//maximal value
      }
      return static_cast<uint64_t>(double(value) * Range.second / Range.first);
    }
  private:
    std::pair<uint64_t, uint64_t> Range;
    std::pair<std::size_t, std::size_t> RangeBits;
  };
}
#endif //MATH_SCALE_H_DEFINED
