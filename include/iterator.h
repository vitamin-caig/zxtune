/**
*
* @file     iterator.h
* @brief    Iterator interfaces
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __ITERATOR_H_DEFINED__
#define __ITERATOR_H_DEFINED__

#include <cassert>
#include <iterator>
#include <memory>

//! @brief Cycled iterator implementation
//! @code
//! std::vector<T> values;
//! //.. fill values
//! CycledIterator<T*> ptr_iterator(&values.front(), &values.back() + 1);
//! CycledIterator<std::vector<T>::const_iterator> it_iterator(values.begin(), values.end());
//! @endcode
//! @invariant Distance between input iterators is more than 1
template<class C>
class CycledIterator
{
public:
  CycledIterator() : Begin(), End(), Cur()
  {
  }

  CycledIterator(C start, C stop) : Begin(start), End(stop), Cur(start)
  {
    assert(std::distance(Begin, End) > 0);
  }

  CycledIterator(const CycledIterator<C>& rh) : Begin(rh.Begin), End(rh.End), Cur(rh.Cur)
  {
    assert(std::distance(Begin, End) > 0);
  }

  const CycledIterator<C>& operator = (const CycledIterator<C>& rh)
  {
    Begin = rh.Begin;
    End = rh.End;
    Cur = rh.Cur;
    assert(std::distance(Begin, End) > 0);
    return *this;
  }

  CycledIterator<C>& operator ++ ()
  {
    if (End == ++Cur)
    {
      Cur = Begin;
    }
    return *this;
  }

  CycledIterator<C>& operator -- ()
  {
    if (Begin == Cur)
    {
      Cur = End;
    }
    --Cur;
    return *this;
  }

  typename std::iterator_traits<C>::reference operator * () const
  {
    return *Cur;
  }

  typename std::iterator_traits<C>::pointer operator ->() const
  {
    return &*Cur;
  }

  CycledIterator<C> operator + (std::size_t dist) const
  {
    const std::size_t len = std::distance(Begin, End);
    const std::size_t pos = std::distance(Begin, Cur);
    CycledIterator<C> res(Begin, End);
    const std::size_t newPos = (pos + dist) % len;
    std::advance(res.Cur, newPos);
    return res;
  }

  CycledIterator<C> operator - (std::size_t dist) const
  {
    const std::size_t len = std::distance(Begin, End);
    const std::size_t pos = std::distance(Begin, Cur);
    CycledIterator<C> res(Begin, End);
    const std::size_t newPos = len + (std::ptrdiff_t(pos) - dist % len);
    std::advance(res.Cur, newPos);
    return res;
  }
private:
  C Begin;
  C End;
  C Cur;
};

//! @brief Range iterator implementation
//! @code
//! for (RangeIterator<const T*> iter = ...; iter; ++iter)
//! {
//!   ...
//!   iter->...
//!   ...
//!   ... = *iter
//! }
//! @endcode
template<class C>
class RangeIterator
{
  void TrueFunc() const
  {
  }
public:
  RangeIterator(C from, C to)
    : Cur(from), Lim(to)
  {
  }

  typedef void (RangeIterator<C>::*BoolType)() const;

  operator BoolType () const
  {
    return Cur != Lim ? &RangeIterator<C>::TrueFunc : 0;
  }

  typename std::iterator_traits<C>::reference operator * () const
  {
    assert(Cur != Lim);
    return *Cur;
  }

  typename std::iterator_traits<C>::pointer operator -> () const
  {
    assert(Cur != Lim);
    return &*Cur;
  }

  RangeIterator<C>& operator ++ ()
  {
    assert(Cur != Lim);
    ++Cur;
    return *this;
  }
private:
  C Cur;
  const C Lim;
};

//! @brief Iterator pattern implementation
template<class T>
class ObjectIterator
{
  class Stub : public ObjectIterator
  {
  public:
    virtual bool IsValid() const
    {
      return false;
    }

    virtual T Get() const
    {
      return T();
    }

    virtual void Next()
    {
      assert(!"Should not be called");
    }
  };

public:
  typedef typename std::auto_ptr<ObjectIterator<T> > Ptr;

  //! Virtual destructor
  virtual ~ObjectIterator() {}

  //! Check if accessor is valid
  virtual bool IsValid() const = 0;
  //! Getting stored value
  //! @invariant Should be called only on valid accessors)
  virtual T Get() const = 0;
  //! Moving to next stored value
  virtual void Next() = 0;

  static Ptr CreateStub()
  {
    return Ptr(new Stub());
  }
};

template<class I>
class RangedObjectIteratorAdapter : public ObjectIterator<typename std::iterator_traits<I>::value_type>
{
public:
  explicit RangedObjectIteratorAdapter(I from, I to)
    : Range(from, to)
  {
  }

  virtual bool IsValid() const
  {
    return Range;
  }

  virtual typename std::iterator_traits<I>::value_type Get() const
  {
    assert(Range);
    return *Range;
  }

  virtual void Next()
  {
    assert(Range);
    ++Range;
  }
private:
  RangeIterator<I> Range;
};

template<class I>
typename RangedObjectIteratorAdapter<I>::Ptr CreateRangedObjectIteratorAdapter(I from, I to)
{
  return typename RangedObjectIteratorAdapter<I>::Ptr(new RangedObjectIteratorAdapter<I>(from, to));
}

#endif //__ITERATOR_H_DEFINED__
