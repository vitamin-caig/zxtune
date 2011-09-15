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

namespace
{
  using namespace Binary;

  typedef boost::shared_ptr<Dump> SharedDump;

  //implementation of DataContainer based on shared array data
  class SharedDumpContainer : public Container
  {
  public:
    SharedDumpContainer(const SharedDump& arr, std::size_t offset, std::size_t size)
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
      return boost::make_shared<SharedDumpContainer>(Buffer, Offset + offset, size);
    }
  private:
    const SharedDump Buffer;
    const std::size_t Offset;
    const std::size_t Length;
  };
}

namespace Binary
{
  //construct container from data dump. Use memcpy since shared_array is less usable in common code
  Container::Ptr CreateContainer(const Dump& data)
  {
    const SharedDump buffer(new Dump(data));
    const std::size_t size = data.size() * sizeof(data.front());
    return boost::make_shared<SharedDumpContainer>(buffer, 0, size);
  }

  Container::Ptr CreateContainer(const void* data, std::size_t size)
  {
    const SharedDump buffer(new Dump(size / sizeof(Dump::value_type)));
    std::memcpy(&buffer->front(), data, size);
    return boost::make_shared<SharedDumpContainer>(buffer, 0, size);
  }

  Container::Ptr CreateContainer(std::auto_ptr<Dump> data)
  {
    const SharedDump buffer(data);
    const std::size_t size = buffer->size() * sizeof(buffer->front());
    return boost::make_shared<SharedDumpContainer>(buffer, 0, size);
  }
}
