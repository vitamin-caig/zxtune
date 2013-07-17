/**
*
* @file      binary/data_builder.h
* @brief     Builder for binary data
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef BINARY_DATA_BUILDER_H_DEFINED
#define BINARY_DATA_BUILDER_H_DEFINED

//library includes
#include <binary/container_factories.h>

namespace Binary
{
  class DataBuilder
  {
  public:
    DataBuilder()
      : Content(new Dump())
    {
    }

    explicit DataBuilder(std::size_t reserve)
      : Content(new Dump())
    {
      Content->reserve(reserve);
    }

    template<class T>
    T* Add()
    {
      return static_cast<T*>(Allocate(sizeof(T)));
    }

    template<class T>
    void Add(const T& val)
    {
      *static_cast<T*>(Allocate(sizeof(T))) = val;
    }

    void* Allocate(std::size_t size)
    {
      const std::size_t curSize = Content->size();
      Content->resize(curSize + size);
      return GetAddress(curSize);
    }

    void AddCString(const std::string& str)
    {
      const std::size_t size = str.size();
      char* const dst = static_cast<char*>(Allocate(size + 1));
      std::copy(str.begin(), str.end(), dst);
      dst[size] = 0;
    }

    void Trim(std::size_t size)
    {
      Content->resize(size);
    }

    void CaptureResult(Dump& res)
    {
      Content->swap(res);
      Content.reset();
    }

    Container::Ptr CaptureResult()
    {
      return CreateContainer(Content);
    }
  private:
    void* GetAddress(std::size_t addr) const
    {
      return &Content->front() + addr;
    }
  private:
    std::auto_ptr<Dump> Content;
  };
}

#endif //BINARY_DATA_BUILDER_H_DEFINED
