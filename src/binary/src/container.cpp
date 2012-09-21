/*
Abstract:
  Data container implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <binary/container_factories.h>
//std includes
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>
#include <boost/static_assert.hpp>

namespace
{
  inline const void* GetPointer(const Dump& val, std::size_t offset)
  {
    return &val[offset];
  }

  inline const void* GetPointer(const Binary::Data& val, std::size_t offset)
  {
    return static_cast<const uint8_t*>(val.Start()) + offset;
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
      size = std::min(size, Length - offset);
      return boost::make_shared<SharedContainer<Value> >(Buffer, Offset + offset, size);
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
  BOOST_STATIC_ASSERT(sizeof(Dump::value_type) == 1);

  //construct container from data dump. Use memcpy since shared_array is less usable in common code
  Container::Ptr CreateContainer(const void* data, std::size_t size)
  {
    if (const uint8_t* byteData = static_cast<const uint8_t*>(data))
    {
      const boost::shared_ptr<Dump> buffer = boost::make_shared<Dump>(byteData, byteData + size);
      return CreateContainer(buffer, 0, size);
    }
    else
    {
      return Container::Ptr();
    }
  }

  Container::Ptr CreateContainer(std::auto_ptr<Dump> data)
  {
    const boost::shared_ptr<const Dump> buffer(data);
    const std::size_t size = buffer ? buffer->size() : 0;
    return CreateContainer(buffer, 0, size);
  }

  Container::Ptr CreateContainer(Data::Ptr data)
  {
    if (data)
    {
      return boost::make_shared<SharedContainer<Data::Ptr> >(data, 0, data->Size());
    }
    else
    {
      return Container::Ptr();
    }
  }

  Container::Ptr CreateContainer(boost::shared_ptr<const Dump> data, std::size_t offset, std::size_t size)
  {
    if (size && data && data->size() >= offset + size)
    {
      return boost::make_shared<SharedContainer<boost::shared_ptr<const Dump> > >(data, offset, size);
    }
    else
    {
      return Container::Ptr();
    }
  }
}
