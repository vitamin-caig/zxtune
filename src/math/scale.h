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
//std includes
#include <cassert>

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

  template<class T1, class T2>
  inline std::pair<T1, T2> OptimizeRatio(T1 first, T2 second)
  {
    assert(second != 0);
    while (0 == ((first | second) & 1))
    {
      first /= T1(2);
      second /= T2(2);
    }
    return std::make_pair(first, second);
  }

  inline uint64_t Scale(uint64_t value, uint64_t inRange, uint64_t outRange)
  {
    //really reverseBits(significantBits(val))
    const uint64_t unsafeScaleMask = HiBitsMask<uint64_t>(Log2(outRange));
    if (0 == (value & unsafeScaleMask))
    {
      return (value * outRange) / inRange;
    }
    else
    {
      const std::pair<uint64_t, uint64_t> valIn = OptimizeRatio(value, inRange);
      const std::pair<uint64_t, uint64_t> outIn = OptimizeRatio(outRange, valIn.second);
      return outIn.second != inRange
          ? Scale(valIn.first, outIn.second, outIn.first)
          : static_cast<uint64_t>(double(value) * outRange / inRange);
    }
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
      , UnsafeScaleMask(HiBitsMask<uint64_t>(Log2(Range.second)))
    {
    }

    ScaleFunctor(const ScaleFunctor<uint64_t>& rh)
      : Range(rh.Range)
      , UnsafeScaleMask(rh.UnsafeScaleMask)
    {
    }

    uint64_t operator()(uint64_t value) const
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
}
#endif //MATH_SCALE_H_DEFINED
