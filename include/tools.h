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

#include <string_type.h>

#include <boost/mpl/if.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_pointer.hpp>

#include <iterator>
#include <algorithm>

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

//! @brief Cycled iterator implementation
//! @code
//! std::vector<T> values;
//! //.. fill values
//! cycled_iterator<T*> ptr_iterator(&values.front(), &values.back() + 1);
//! cycled_iterator<std::vector<T>::const_iterator> it_iterator(values.begin(), values.end());
//! @endcode
//! @invariant Distance between input iterators is more than 1
template<class C>
class cycled_iterator
{
public:
  cycled_iterator() : begin(), end(), cur()
  {
  }

  cycled_iterator(C start, C stop) : begin(start), end(stop), cur(start)
  {
    assert(std::distance(begin, end) > 0);
  }

  cycled_iterator(const cycled_iterator<C>& rh) : begin(rh.begin), end(rh.end), cur(rh.cur)
  {
    assert(std::distance(begin, end) > 0);
  }

  const cycled_iterator<C>& operator = (const cycled_iterator<C>& rh)
  {
    begin = rh.begin;
    end = rh.end;
    cur = rh.cur;
    assert(std::distance(begin, end) > 0);
    return *this;
  }

  bool operator == (const cycled_iterator<C>& rh) const
  {
    return begin == rh.begin && end == rh.end && cur == rh.cur;
  }

  bool operator != (const cycled_iterator<C>& rh) const
  {
    return !(*this == rh);
  }

  cycled_iterator<C>& operator ++ ()
  {
    if (end == ++cur)
    {
      cur = begin;
    }
    return *this;
  }

  cycled_iterator<C>& operator -- ()
  {
    if (begin == cur)
    {
      cur = end;
    }
    --cur;
    return *this;
  }

  typename std::iterator_traits<C>::reference operator * () const
  {
    return *cur;
  }

  typename std::iterator_traits<C>::pointer operator ->() const
  {
    return &*cur;
  }
private:
  C begin;
  C end;
  C cur;
};

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

#endif //__TOOLS_H_DEFINED__
