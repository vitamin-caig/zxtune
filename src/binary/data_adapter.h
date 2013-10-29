/**
*
* @file
*
* @brief  Binary::Data lightweight adapter
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/data.h>

namespace Binary
{
  class DataAdapter : public Data
  {
  public:
    DataAdapter(const void* start, std::size_t size)
      : StartValue(start)
      , SizeValue(size)
    {
    }

    virtual const void* Start() const
    {
      return StartValue;
    }

    virtual std::size_t Size() const
    {
      return SizeValue;
    }
  private:
    const void* const StartValue;
    const std::size_t SizeValue;
  };
}
