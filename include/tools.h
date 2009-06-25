#ifndef __TOOLS_H_DEFINED__
#define __TOOLS_H_DEFINED__

#include <types.h>

#include <boost/mpl/if.hpp>
#include <boost/call_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/remove_pointer.hpp>

#include <iterator>
#include <algorithm>

template<class T, std::size_t D>
inline std::size_t ArraySize(const T (&)[D])
{
  return D;
}

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


template<class T, class F>
inline T safe_ptr_cast(F from)
{
  using namespace boost;
  BOOST_STATIC_ASSERT(is_pointer<F>::value);
  BOOST_STATIC_ASSERT(is_pointer<T>::value);
  typedef typename mpl::if_c<is_const<typename remove_pointer<T>::type>::value, const void*, void*>::type MidType;
  return static_cast<T>(static_cast<MidType>(from));
}

template<class T>
inline String string_cast(const T& val)
{
  OutStringStream str;
  str << val;
  return str.str();
}

template<class P, class T>
inline String string_cast(const P& p, const T& val)
{
  OutStringStream str;
  str << p << val;
  return str.str();
}

template<class T>
inline String string_cast(std::ios_base& (*M)(std::ios_base&), const T& val)
{
  OutStringStream str;
  str << M << val;
  return str.str();
}

template<class T>
inline T string_cast(const String& s)
{
  T val = T();
  InStringStream str(s);
  if (str >> val)
  {
    return val;
  }
  return T();//TODO
}

template<class T>
inline T string_cast(const String& s, T def)
{
  T val = T();
  InStringStream str(s);
  if (str >> val)
  {
    return val;
  }
  return def;//TODO
}


template<class C>
class cycled_iterator
{
public:
  cycled_iterator() : begin(), end(), cur()
  {
  }

  cycled_iterator(C start, C stop) : begin(start), end(stop), cur(start)
  {
  }

  cycled_iterator(const cycled_iterator<C>& rh) : begin(rh.begin), end(rh.end), cur(rh.cur)
  {
  }

  const cycled_iterator<C>& operator = (const cycled_iterator<C>& rh)
  {
    begin = rh.begin;
    end = rh.end;
    cur = rh.cur;
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

template<class T>
inline T clamp(T val, T min, T max)
{
  return std::min<T>(std::max<T>(val, min), max);
}

template<class T>
inline bool in_range(typename boost::call_traits<T>::param_type val, 
                     typename boost::call_traits<T>::param_type min, 
                     typename boost::call_traits<T>::param_type max)
{
  return val >= min && val <= max;
}

template<class T>
inline T align(T val, std::size_t alignment)
{
  return alignment * ((val - 1) / alignment + 1);
}

#endif //__TOOLS_H_DEFINED__
