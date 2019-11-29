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

//local includes
#include "binary/data.h" //TODO: remove
//common includes
#include <contract.h>
//std includes
#include <type_traits>
#include <vector>

namespace Binary
{
  class View
  {
  public:
    View(const void* start, std::size_t size)
      : Begin(start)
      , Length(size)
    {
      Require(start != nullptr);
      Require(size != 0);
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
    /*explicit*/View(const Data& data)
      : View(data.Start(), data.Size())
    {
    }

    bool operator == (View rh) const
    {
      return Begin == rh.Begin && Length == rh.Length;
    }

    const void* Start() const
    {
      return Begin;
    }

    std::size_t Size() const
    {
      return Length;
    }
  private:
    const void* const Begin;
    const std::size_t Length;
  };
}
