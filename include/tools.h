#ifndef __TOOLS_H_DEFINED__
#define __TOOLS_H_DEFINED__

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
inline T safe_ptr_cast(F* from)
{
  return static_cast<T>(static_cast<void*>(from));
}

template<class T, class F>
inline T safe_ptr_cast(const F* from)
{
  return static_cast<T>(static_cast<const void*>(from));
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

  cycled_iterator<C>& operator = (const cycled_iterator<C>& rh)
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
inline T align(T val, std::size_t alignment)
{
  return alignment * ((val - 1) / alignment + 1);
}

template<class T>
class Optional
{
public:
  Optional() : Value(), Null(true)
  {
  }

  /*explicit*/Optional(const T& val) : Value(val), Null(false)
  {
  }

  Optional(const Optional<T>& rh) : Value(rh.Value), Null(rh.Null)
  {
  }

  operator T& ()
  {
    return Value;
  }

  operator const T& () const
  {
    return Value;
  }

  Optional<T>& operator = (const T& val)
  {
    Value = val;
    Null = false;
    return *this;
  }

  bool IsNull() const
  {
    return Null;
  }

  void Reset()
  {
    Null = true;
  }
private:
  T Value;
  bool Null;
};

#endif //__TOOLS_H_DEFINED__
