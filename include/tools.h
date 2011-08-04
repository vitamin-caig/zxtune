/**
*
* @file     tools.h
* @brief    Commonly used tools
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __TOOLS_H_DEFINED__
#define __TOOLS_H_DEFINED__

//local includes
#include <types.h>
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

//! @brief Universal replacement of bunch abs/fabs/labs and other
template<class T>
inline T absolute(T val)
{
  return val >= 0 ? val : -val;
}

//! @brief Stub deleter for shared pointers
template<class T>
class NullDeleter
{
public:
  void operator()(T*) {}
};

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

#endif //__TOOLS_H_DEFINED__
