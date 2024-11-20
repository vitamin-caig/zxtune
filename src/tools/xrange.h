/**
 *
 * @file
 *
 * @brief  Python-alike xrange implementation to use in for-ranged loops
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <algorithm>
#include <iterator>
#include <type_traits>

namespace XrangePrivate
{
  template<typename T>
  class SimpleRange
  {
  public:
    SimpleRange(T start, T finish)
      : Start(start)
      , Finish(std::max(start, finish))
    {
      static_assert(std::is_integral<T>::value || std::is_pointer<T>::value, "T should be integral type or pointer");
    }

    class Iterator
    {
    public:
      using value_type = T;
      using difference_type = decltype(T() - T());
      using pointer = const T*;
      using reference = const T&;
      using iterator_category = std::random_access_iterator_tag;

      explicit Iterator(T value)
        : Value(value)
      {}

      T operator*() const
      {
        return Value;
      }

      bool operator!=(const Iterator& rh) const
      {
        return Value != rh.Value;
      }

      bool operator==(const Iterator& rh) const
      {
        return Value == rh.Value;
      }

      Iterator& operator++()
      {
        ++Value;
        return *this;
      }

      difference_type operator-(const Iterator& rh) const
      {
        return Value - rh.Value;
      }

      template<class P>
      Iterator operator+(const P& p) const
      {
        return Iterator(Value + p);
      }

      template<class P>
      Iterator& operator+=(const P& p) const
      {
        Value += p;
        return *this;
      }

      template<class P>
      Iterator operator-(const P& p) const
      {
        return Iterator(Value - p);
      }

    private:
      T Value;
    };

    using value_type = T;
    using iterator = Iterator;
    using const_iterator = Iterator;

    Iterator begin() const
    {
      return Iterator(Start);
    }

    Iterator end() const
    {
      return Iterator(Finish);
    }

    T size() const
    {
      return Finish - Start;
    }

    template<class Container>
    operator Container() const
    {
      return Container(begin(), end());
    }

  private:
    const T Start;
    const T Finish;
  };
}  // namespace XrangePrivate

template<class T>
::XrangePrivate::SimpleRange<T> xrange(T finish)
{
  return {T(), finish};
}

template<class T>
::XrangePrivate::SimpleRange<T> xrange(T start, T finish)
{
  return {start, finish};
}
