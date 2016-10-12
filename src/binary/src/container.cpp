/**
*
* @file
*
* @brief  Data container implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
//std includes
#include <algorithm>
#include <cassert>
#include <cstring>

namespace Binary
{
  inline const void* GetPointer(const Dump& val, std::size_t offset)
  {
    return &val[offset];
  }

  inline const void* GetPointer(const Binary::Data& val, std::size_t offset)
  {
    return static_cast<const uint8_t*>(val.Start()) + offset;
  }

  inline const void* GetPointer(const uint8_t& val, std::size_t offset)
  {
    return &val + offset;
  }

  template<class Value>
  class SharedContainer : public Binary::Container
  {
  public:
    SharedContainer(Value arr, std::size_t offset, std::size_t size)
      : Buffer(arr)
      , Address(GetPointer(*arr, offset))
      , Offset(offset)
      , Length(size)
    {
      assert(Length);
    }

    virtual const void* Start() const
    {
      return Address;
    }

    virtual std::size_t Size() const
    {
      return Length;
    }

    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      assert(offset + size <= Length);
      if (size && offset < Length)
      {
        size = std::min(size, Length - offset);
        return MakePtr<SharedContainer<Value> >(Buffer, Offset + offset, size);
      }
      else
      {
        return Ptr();
      }
    }
  private:
    const Value Buffer;
    const void* const Address;
    const std::size_t Offset;
    const std::size_t Length;
  };
}

namespace Binary
{
  static_assert(sizeof(Dump::value_type) == 1, "Invalid size for Dump::value_type");

  Container::Ptr CreateContainer(const void* data, std::size_t size)
  {
    if (const uint8_t* byteData = size ? static_cast<const uint8_t*>(data) : 0)
    {
      const std::shared_ptr<const Dump> buffer(new Dump(byteData, byteData + size));
      return CreateContainer(buffer, 0, size);
    }
    else
    {
      return Container::Ptr();
    }
  }

  Container::Ptr CreateNonCopyContainer(const void* data, std::size_t size)
  {
    if (const uint8_t* byteData = size ? static_cast<const uint8_t*>(data) : 0)
    {
      return MakePtr<SharedContainer<const uint8_t*> >(byteData, 0, size);
    }
    else
    {
      return Container::Ptr();
    }
  }

  Container::Ptr CreateContainer(std::unique_ptr<Dump> data)
  {
    const std::shared_ptr<const Dump> buffer(data.release());
    const std::size_t size = buffer ? buffer->size() : 0;
    return CreateContainer(buffer, 0, size);
  }

  Container::Ptr CreateContainer(Data::Ptr data)
  {
    //cover downcasting
    if (const Container::Ptr asContainer = std::dynamic_pointer_cast<const Container>(data))
    {
      return asContainer;
    }
    else if (data && data->Size())
    {
      return MakePtr<SharedContainer<Data::Ptr> >(data, 0, data->Size());
    }
    else
    {
      return Container::Ptr();
    }
  }

  Container::Ptr CreateContainer(std::shared_ptr<const Dump> data, std::size_t offset, std::size_t size)
  {
    if (size && data && data->size() >= offset + size)
    {
      return MakePtr<SharedContainer<std::shared_ptr<const Dump> > >(data, offset, size);
    }
    else
    {
      return Container::Ptr();
    }
  }
}
