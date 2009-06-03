#include "container.h"
#include "location.h"

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#include <fstream>

namespace
{
  using namespace ZXTune::IO;

  //TODO: check results of fileops

  //Assume 1Mb is enough for caching
  const std::size_t MAX_CACHED_SIZE = 1024 * 1024;

  class MemoryContainerImpl : public DataContainer
  {
  public:
    MemoryContainerImpl(const boost::shared_array<uint8_t>& arr, std::size_t start, std::size_t size)
      : Array(arr)
      , Start(start)
      , Length(size)
    {
      assert(arr.get() && size);
    }

    virtual std::size_t Size() const
    {
      return Length;
    }

    virtual const void* Data() const
    {
      return Array.get() + Start;
    }

    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      assert(offset + size <= Start + Length);
      return Ptr(new MemoryContainerImpl(Array, Start + offset, size));
    }
  private:
    const boost::shared_array<uint8_t> Array;
    const std::size_t Start;
    const std::size_t Length;
  };

  class FileContainerImpl : public DataContainer
  {
  public:
    typedef boost::shared_ptr<std::ifstream> SharedStream;
    FileContainerImpl(SharedStream stream, std::size_t start, std::size_t size)
      : Stream(stream), Start(start), Length(size)
    {
      assert(stream.get() && size);
    }

    virtual std::size_t Size() const
    {
      return Length;
    }

    virtual const void* Data() const
    {
      DoCaching();
      assert(Cache.get());
      return Cache.get();
    }

    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      assert(offset + size <= Start + Length);
      return Ptr(Cache.get() ?
        static_cast<DataContainer*>(new MemoryContainerImpl(Cache, offset, size))
        :
        static_cast<DataContainer*>(new FileContainerImpl(Stream, Start + offset, size)));
    }
  private:
    void DoCaching() const
    {
      if (!Cache.get())
      {
        assert(Length <= MAX_CACHED_SIZE);
        Stream->seekg(Start);
        Cache.reset(new uint8_t[Length]);
        Stream->read(safe_ptr_cast<char*>(Cache.get()), Length);
      }
    }
  private:
    SharedStream Stream;
    std::size_t Start;
    std::size_t Length;
    mutable boost::shared_array<uint8_t> Cache;
  };
}

namespace ZXTune
{
  namespace IO
  {
    DataContainer::Ptr DataContainer::Create(const String& filename)
    {
      FileContainerImpl::SharedStream stream(new std::ifstream(ExtractFSPath(filename).c_str(), std::ios::binary));
      if (!*stream)
      {
        throw 1;//TODO
      }
      //std::cout << "Reading " << filename << std::endl;
      stream->seekg(0, std::ios::end);
      return DataContainer::Ptr(new FileContainerImpl(stream, 0, stream->tellg()));
    }

    DataContainer::Ptr DataContainer::Create(const Dump& data)
    {
      const std::size_t inBytes(data.size() * sizeof(data.front()));
      boost::shared_array<uint8_t> tmp(new uint8_t[inBytes]);
      std::memcpy(tmp.get(), &data[0], inBytes);
      return DataContainer::Ptr(new MemoryContainerImpl(tmp, 0, inBytes));
    }
  }
}
