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

    const void* Start() const override
    {
      return StartValue;
    }

    std::size_t Size() const override
    {
      return SizeValue;
    }
  private:
    const void* const StartValue;
    const std::size_t SizeValue;
  };
}
