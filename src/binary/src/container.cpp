/*
Abstract:
  Data container implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <binary/container.h>
//std includes
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>
#include <boost/static_assert.hpp>

namespace
{
  using namespace Binary;

  typedef boost::shared_ptr<const Dump> DumpPtr;

  //implementation of DataContainer based on shared array data
  class SharedDumpContainer : public Container
  {
  public:
    SharedDumpContainer(const DumpPtr& arr, std::size_t offset, std::size_t size)
      : Buffer(arr), Offset(offset), Length(size)
    {
      assert(Length);
    }

    virtual std::size_t Size() const
    {
      return Length;
    }

    virtual const void* Data() const
    {
      return &(*Buffer)[Offset];
    }

    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      assert(offset + size <= Length);
      size = std::min(size, Length - offset);
      return CreateContainer(Buffer, Offset + offset, size);
    }
  private:
    const DumpPtr Buffer;
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
    if (data.get())
    {
      const DumpPtr buffer(data);
      return CreateContainer(buffer, 0, buffer->size());
    }
    else
    {
      return Container::Ptr();
    }
  }

  Container::Ptr CreateContainer(boost::shared_ptr<const Dump> data, std::size_t offset, std::size_t size)
  {
    if (data && data->size() >= offset + size)
    {
      return boost::make_shared<SharedDumpContainer>(data, offset, size);
    }
    else
    {
      return Container::Ptr();
    }
  }
}
