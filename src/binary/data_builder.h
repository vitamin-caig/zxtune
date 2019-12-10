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
#include <binary/view.h>
//std includes
#include <cstring>
#include <type_traits>

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

    template<class T, typename std::enable_if<!std::is_pointer<T>::value, int>::type = 0>
    T& Add()
    {
      return *static_cast<T*>(Allocate(sizeof(T)));
    }

    template<class T, typename std::enable_if<std::is_trivial<T>::value && !std::is_pointer<T>::value, int>::type = 0>
    void Add(T val)
    {
      *static_cast<T*>(Allocate(sizeof(T))) = val;
    }
    
    void Add(View data)
    {
      std::memcpy(Allocate(data.Size()), data.Start(), data.Size());
    }

    void* Allocate(std::size_t size)
    {
      const std::size_t curSize = Content->size();
      Content->resize(curSize + size);
      return Get(curSize);
    }

    void AddCString(const StringView str)
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
      return Content->data() + offset;
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
      return CreateContainer(std::move(Content));
    }
  private:
    std::unique_ptr<Dump> Content;
  };
}
