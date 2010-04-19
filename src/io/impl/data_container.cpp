/*
Abstract:
  Data container implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <io/container.h>
//std includes
#include <cstring>
//boost includes
#include <boost/shared_array.hpp>

namespace
{
  using namespace ZXTune::IO;

  typedef boost::shared_array<uint8_t> SharedArray;

  //implementation of DataContainer based on shared array data
  class DataContainerImpl : public DataContainer
  {
  public:
    DataContainerImpl(const SharedArray& arr, std::size_t offset, std::size_t size)
      : Buffer(arr), Offset(offset), Length(size)
    {
    }

    virtual std::size_t Size() const
    {
      return Length;
    }

    virtual const void* Data() const
    {
      return Buffer.get() + Offset;
    }

    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      assert(offset + size <= Length);
      size = std::min(size, Length - offset);
      return Ptr(new DataContainerImpl(Buffer, Offset + offset, size));
    }
  private:
    const SharedArray Buffer;
    const std::size_t Offset;
    const std::size_t Length;
  };
}

namespace ZXTune
{
  namespace IO
  {
    //construct container from data dump. Use memcpy since shared_array is less usable in common code
    DataContainer::Ptr CreateDataContainer(const Dump& data)
    {
      const std::size_t allsize = data.size() * sizeof(data.front());
      SharedArray buffer(new uint8_t[allsize]);
      std::memcpy(buffer.get(), &data[0], allsize);
      return DataContainer::Ptr(new DataContainerImpl(buffer, 0, allsize));
    }
  }
}
