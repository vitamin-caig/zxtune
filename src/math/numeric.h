/**
*
* @file     numeric.h
* @brief    Numeric operations
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef MATH_NUMERIC_H_DEFINED
#define MATH_NUMERIC_H_DEFINED

//std includes
#include <algorithm>
//boost includes
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>

namespace Math
{
  //! @brief Clamping value into specified range
  template<class T>
  inline T Clamp(T val, T min, T max)
  {
    BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value);
    return std::min<T>(std::max<T>(val, min), max);
  }

  //! @brief Checking if value is in specified range (inclusive)
  template<class T>
  inline bool InRange(T val, T min, T max)
  {
    BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value);
    return val >= min && val <= max;
  }

  //! @brief Align numeric with specified alignment
  template<class T>
  inline T Align(T val, T alignment)
  {
    BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
    return alignment * ((val - 1) / alignment + 1);
  }

  //! @brief Universal replacement of bunch abs/fabs/labs and other
  template<class T>
  inline T Absolute(T val)
  {
    BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value);
    return val >= 0 ? val : -val;
  }
}

#endif //MATH_NUMERIC_H_DEFINED
