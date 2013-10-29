/**
*
* @file
*
* @brief  Builder for binary data
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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
    T& Add()
    {
      return *static_cast<T*>(Allocate(sizeof(T)));
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
      return Get(curSize);
    }

    void AddCString(const std::string& str)
    {
      const std::size_t size = str.size();
      char* const dst = static_cast<char*>(Allocate(size + 1));
      std::copy(str.begin(), str.end(), dst);
      dst[size] = 0;
    }

    std::size_t Size() const
    {
      return Content->size();
    }

    void* Get(std::size_t offset) const
    {
      return &Content->front() + offset;
    }

    template<class T>
    T& Get(std::size_t offset) const
    {
      return *static_cast<T*>(Get(offset));
    }

    void Resize(std::size_t size)
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
    std::auto_ptr<Dump> Content;
  };
}
