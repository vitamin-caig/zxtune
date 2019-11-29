/**
*
* @file
*
* @brief  Memory region non-owning reference
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//std includes
#include <type_traits>
#include <vector>

namespace Binary
{
  class View
  {
  private:
    View()
     : View(nullptr, 0)
    {
    }
  public:
    View(const void* start, std::size_t size)
      : Begin(size ? start : nullptr)
      , Length(Begin ? size : 0)
    {
    }

    template<class T>
    View(const std::vector<T>& data)
      : View(data.data(), data.size() * sizeof(T))
    {
    }

    //is_trivially_copyable is not implemented in windows mingw
    template<typename T,
             typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value && std::is_compound<T>::value, int>::type = 0>
    View(const T& data)
      : View(&data, sizeof(data))
    {
    }

    //TODO: remove
    /*explicit*/View(const class Data& data);

    const void* Start() const
    {
      return Begin;
    }

    std::size_t Size() const
    {
      return Length;
    }

    explicit operator bool () const
    {
      return Length != 0;
    }

    bool operator == (View rh) const
    {
      return Begin == rh.Begin && Length == rh.Length;
    }

    View SubView(std::size_t offset, std::size_t maxSize = ~0u) const
    {
      if (offset < Length)
      {
        return View(static_cast<const uint8_t*>(Begin) + offset, std::min(maxSize, Length - offset));
      }
      else
      {
        return View();
      }
    }
  private:
    const void* const Begin;
    const std::size_t Length;
  };
}
