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

// library includes
#include <binary/container_factories.h>
#include <binary/dump.h>
#include <binary/view.h>
// std includes
#include <cstring>
#include <type_traits>

namespace Binary
{
  class DataBuilder
  {
  public:
    DataBuilder() {}

    explicit DataBuilder(std::size_t reserve)
    {
      Content.reserve(reserve);
    }

    template<class T, typename std::enable_if<!std::is_pointer<T>::value, int>::type = 0>
    T& Add()
    {
      return *safe_ptr_cast<T*>(Allocate(sizeof(T)));
    }

    template<class T, typename std::enable_if<std::is_trivial<T>::value && !std::is_pointer<T>::value, int>::type = 0>
    void Add(T val)
    {
      static_assert(sizeof(T) == 1 || alignof(T) == 1, "Endian-dependent type");
      std::memcpy(Allocate(sizeof(T)), &val, sizeof(val));
    }

    void AddByte(uint8_t val)
    {
      Content.push_back(val);
    }

    void* Add(View data)
    {
      return std::memcpy(Allocate(data.Size()), data.Start(), data.Size());
    }

    void* Allocate(std::size_t size)
    {
      const std::size_t curSize = Content.size();
      Content.resize(curSize + size);
      return Get(curSize);
    }

    char* AddCString(const StringView str)
    {
      const std::size_t size = str.size();
      char* const dst = static_cast<char*>(Allocate(size + 1));
      std::copy(str.begin(), str.end(), dst);
      dst[size] = 0;
      return dst;
    }

    std::size_t Size() const
    {
      return Content.size();
    }

    void* Get(std::size_t offset)
    {
      return Content.data() + offset;
    }

    template<class T>
    T& Get(std::size_t offset)
    {
      return *safe_ptr_cast<T*>(Get(offset));
    }

    View GetView() const
    {
      return View(Content);
    }

    void Resize(std::size_t size)
    {
      Content.resize(size);
    }

    void CaptureResult(Dump& res)
    {
      res = std::move(Content);
    }

    Container::Ptr CaptureResult()
    {
      return CreateContainer(std::make_unique<Dump>(std::move(Content)));
    }

  private:
    Dump Content;
  };
}  // namespace Binary
