/**
 *
 * @file
 *
 * @brief  Numeric operations
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <algorithm>
#include <type_traits>

namespace Math
{
  //! @brief Clamping value into specified range
  template<class T>
  inline T Clamp(T val, T min, T max)
  {
    static_assert(std::is_arithmetic<T>::value, "Should be arithmetic");
    return std::min<T>(std::max<T>(val, min), max);
  }

  //! @brief Checking if value is in specified range (inclusive)
  template<class T>
  inline bool InRange(T val, T min, T max)
  {
    static_assert(std::is_arithmetic<T>::value, "Should be arithmetic");
    return val >= min && val <= max;
  }

  template<class T>
  class InRangeFunctor
  {
  public:
    InRangeFunctor(T min, T max)
      : Min(min)
      , Max(max)
    {}

    bool operator()(T val) const
    {
      return InRange(val, Min, Max);
    }

  private:
    const T Min;
    const T Max;
  };

  template<class T>
  InRangeFunctor<T> InRange(T min, T max)
  {
    return InRangeFunctor<T>(min, max);
  }

  template<class T>
  class NotInRangeFunctor
  {
  public:
    NotInRangeFunctor(T min, T max)
      : Min(min)
      , Max(max)
    {}

    bool operator()(T val) const
    {
      return !InRange(val, Min, Max);
    }

  private:
    const T Min;
    const T Max;
  };

  template<class T>
  NotInRangeFunctor<T> NotInRange(T min, T max)
  {
    return NotInRangeFunctor<T>(min, max);
  }

  //! @brief Align numeric with specified alignment
  template<class T>
  inline T Align(T val, T alignment)
  {
    static_assert(std::is_integral<T>::value, "Should be integral");
    return alignment * ((val - 1) / alignment + 1);
  }

  //! @brief Universal replacement of bunch abs/fabs/labs and other
  template<class T>
  inline T Absolute(T val)
  {
    static_assert(std::is_arithmetic<T>::value, "Should be arithmetic");
    return val >= 0 ? val : -val;
  }
}  // namespace Math
