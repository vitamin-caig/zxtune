/**
*
* @file     bitops.h
* @brief    Bits operating and related functions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef MATH_BITOPS_H_DEFINED
#define MATH_BITOPS_H_DEFINED

//local includes
#include <types.h>
//boost includes
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>

namespace Math
{
  //! @brief Counting set bits in integer value
  template<class T>
  inline std::size_t CountBits(T val)
  {
    BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
    std::size_t res = 0;
    while (val)
    {
      if (val & 1)
      {
        ++res;
      }
      val >>= 1;
    }
    return res;
  }

  //! @brief Counting significant bits count
  inline std::size_t Log2(uint32_t val)
  {
    const std::size_t shift4 = (val > 0xffff) << 4;
    val >>= shift4;
    const std::size_t shift3 = (val > 0xff) << 3;
    val >>= shift3;
    const std::size_t shift2 = (val > 0xf) << 2;
    val >>= shift2;
    const std::size_t shift1 = (val > 0x3) << 1;
    val >>= shift1;
    const std::size_t shift0 = val >> 1;
    return 1 + (shift4 | shift3 | shift2 | shift1 | shift0);
  }

  inline std::size_t Log2(uint64_t val)
  {
    const std::size_t shift5 = (val > 0xffffffff) << 5;
    val >>= shift5;
    const std::size_t shift4 = (val > 0xffff) << 4;
    val >>= shift4;
    const std::size_t shift3 = (val > 0xff) << 3;
    val >>= shift3;
    const std::size_t shift2 = (val > 0xf) << 2;
    val >>= shift2;
    const std::size_t shift1 = (val > 0x3) << 1;
    val >>= shift1;
    const std::size_t shift0 = static_cast<std::size_t>(val >> 1);
    return 1 + (shift5 | shift4 | shift3 | shift2 | shift1 | shift0);
  }

  template<class T>
  inline std::size_t Log2(T val)
  {
    BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
    std::size_t res = 0;
    while (val >>= 1)
    {
      ++res;
    }
    return 1 + res;
  }

  template<class T>
  inline T HiBitsMask(std::size_t hiBits)
  {
    return ~((T(1) << (8 * sizeof(T) - hiBits)) - 1);
  }
}

#endif //MATH_BITOPS_H_DEFINED
