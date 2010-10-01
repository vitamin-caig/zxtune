/**
*
* @file     tools.h
* @brief    Commonly used tools
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __TOOLS_H_DEFINED__
#define __TOOLS_H_DEFINED__

//std includes
#include <algorithm>
//boost includes
#include <boost/static_assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_pointer.hpp>

//! @brief Calculating size of fixed-size array
template<class T, std::size_t D>
inline std::size_t ArraySize(const T (&)[D])
{
  return D;
}

//! @fn template<class T, std::size_t D>inline const T* ArrayEnd(const T (&c)[D])
//! @brief Calculating end iterator of fixed-size array

//workaround for ArrayEnd (need for packed structures)
#undef ArrayEnd
#if defined(__GNUC__)
# if __GNUC__ * 100 + __GNUC_MINOR__ > 303
#  define ArrayEnd(a) ((a) + ArraySize(a))
# endif
#endif

#ifndef ArrayEnd
template<class T, std::size_t D>
inline const T* ArrayEnd(const T (&c)[D])
{
  return c + D;
}

template<class T, std::size_t D>
inline T* ArrayEnd(T (&c)[D])
{
  return c + D;
}
#endif

//! @brief Performing safe pointer casting
template<class T, class F>
inline T safe_ptr_cast(F from)
{
  using namespace boost;
  BOOST_STATIC_ASSERT(is_pointer<F>::value);
  BOOST_STATIC_ASSERT(is_pointer<T>::value);
  typedef typename mpl::if_c<is_const<typename remove_pointer<T>::type>::value, const void*, void*>::type MidType;
  return static_cast<T>(static_cast<MidType>(from));
}

//! @brief Clamping value into specified range
template<class T>
inline T clamp(T val, T min, T max)
{
  BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value);
  return std::min<T>(std::max<T>(val, min), max);
}

//! @brief Checking if value is in specified range (inclusive)
template<class T>
inline bool in_range(T val, T min, T max)
{
  BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value);
  return val >= min && val <= max;
}

//! @brief Align numeric with specified alignment
template<class T>
inline T align(T val, T alignment)
{
  BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
  return alignment * ((val - 1) / alignment + 1);
}

//! @brief Counting set bits in integer value
template<class T>
inline T CountBits(T val)
{
  BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
  T res = 0;
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

//! @brief Universal replacement of bunch abs/fabs/labs and other
template<class T>
inline T absolute(T val)
{
  return val >= 0 ? val : -val;
}

#endif //__TOOLS_H_DEFINED__
