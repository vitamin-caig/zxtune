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

//std includes
#include <algorithm>
//boost includes
#include <boost/type_traits/is_arithmetic.hpp>

namespace Math
{
  //! @brief Clamping value into specified range
  template<class T>
  inline T Clamp(T val, T min, T max)
  {
    static_assert(boost::is_arithmetic<T>::value, "Should be arithmetic");
    return std::min<T>(std::max<T>(val, min), max);
  }

  //! @brief Checking if value is in specified range (inclusive)
  template<class T>
  inline bool InRange(T val, T min, T max)
  {
    static_assert(boost::is_arithmetic<T>::value, "Should be arithmetic");
    return val >= min && val <= max;
  }

  //! @brief Align numeric with specified alignment
  template<class T>
  inline T Align(T val, T alignment)
  {
    static_assert(boost::is_integral<T>::value, "Should be integral");
    return alignment * ((val - 1) / alignment + 1);
  }

  //! @brief Universal replacement of bunch abs/fabs/labs and other
  template<class T>
  inline T Absolute(T val)
  {
    static_assert(boost::is_arithmetic<T>::value, "Should be arithmetic");
    return val >= 0 ? val : -val;
  }
}
