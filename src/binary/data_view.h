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

namespace Binary
{
  class DataView
  {
  public:
    DataView(const void* start, std::size_t size)
      : Begin(start)
      , Length(size)
    {
      Require(start != nullptr);
      Require(size != 0);
    }

    //TODO: remove
    /*explicit*/DataView(const Data& data)
      : DataView(data.Start(), data.Size())
    {
    }

    bool operator == (DataView rh) const
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
