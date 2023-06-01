/**
 *
 * @file
 *
 * @brief  Data container implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/container_factories.h>
// std includes
#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>
#include <type_traits>

namespace Binary
{
  inline const void* GetPointer(const Dump* val, std::size_t offset)
  {
    return val->data() + offset;
  }

  inline const void* GetPointer(const Binary::Data* val, std::size_t offset)
  {
    return static_cast<const uint8_t*>(val->Start()) + offset;
  }

  inline const void* GetPointer(const void* val, std::size_t offset)
  {
    return static_cast<const uint8_t*>(val) + offset;
  }

  template<class Value>
  class SharedContainer : public Binary::Container
  {
  public:
    SharedContainer(Value arr, std::size_t offset, std::size_t size)
      : Buffer(std::move(arr))
      , Offset(offset)
      , Length(size)
    {
      assert(Length);
    }

    const void* Start() const override
    {
      return GetPointer(Buffer.get(), Offset);
    }

    std::size_t Size() const override
    {
      return Length;
    }

    Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
    {
      if (size && offset < Length)
      {
        size = std::min(size, Length - offset);
        return MakePtr<SharedContainer<Value> >(Buffer, Offset + offset, size);
      }
      else
      {
        return {};
      }
    }

  private:
    const Value Buffer;
    const std::size_t Offset;
    const std::size_t Length;
  };
}  // namespace Binary

namespace Binary
{
  static_assert(sizeof(Dump::value_type) == 1, "Invalid size for Dump::value_type");

  Container::Ptr CreateContainer(View data)
  {
    const auto size = data.Size();
    if (const uint8_t* byteData = size ? static_cast<const uint8_t*>(data.Start()) : nullptr)
    {
      std::shared_ptr<const Dump> buffer(new Dump(byteData, byteData + size));
      return CreateContainer(std::move(buffer), 0, size);
    }
    else
    {
      return {};
    }
  }

  Container::Ptr CreateContainer(std::unique_ptr<Dump> data)
  {
    std::shared_ptr<const Dump> buffer(data.release());
    const std::size_t size = buffer ? buffer->size() : 0;
    return CreateContainer(std::move(buffer), 0, size);
  }

  Container::Ptr CreateContainer(Data::Ptr data)
  {
    // cover downcasting and special cases
    if (auto asContainer = std::dynamic_pointer_cast<const Container>(data))
    {
      return asContainer;
    }
    else if (const auto size = data->Size())
    {
      return MakePtr<SharedContainer<Data::Ptr> >(std::move(data), 0, size);
    }
    else
    {
      return {};
    }
  }

  Container::Ptr CreateContainer(std::shared_ptr<const Dump> data, std::size_t offset, std::size_t size)
  {
    if (size && data && offset < data->size())
    {
      size = std::min(size, data->size() - offset);
      return MakePtr<SharedContainer<std::shared_ptr<const Dump> > >(std::move(data), offset, size);
    }
    else
    {
      return {};
    }
  }
}  // namespace Binary
