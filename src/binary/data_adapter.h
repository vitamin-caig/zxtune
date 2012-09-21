/**
*
* @file      binary/data_adapter.h
* @brief     Binary::Data lightweight adapter
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef BINARY_DATA_ADAPTER_H_DEFINED
#define BINARY_DATA_ADAPTER_H_DEFINED

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

#endif //BINARY_DATA_ADAPTER_H_DEFINED
