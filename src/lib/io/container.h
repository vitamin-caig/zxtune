#ifndef __CONTAINER_H_DEFINED__
#define __CONTAINER_H_DEFINED__

#include <tools.h>
#include <types.h>

#include <memory>

namespace ZXTune
{
  namespace IO
  {
    class DataContainer
    {
    public:
      typedef std::auto_ptr<DataContainer> Ptr;
      virtual ~DataContainer()
      {
      }

      virtual std::size_t Size() const = 0;
      virtual const void* Data() const = 0;
      virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const = 0;

      //helper
      template<class T>
      const T& operator [] (std::size_t idx) const
      {
        assert(0 == Size() % sizeof(T) && idx < Size() / sizeof(T));
        return *(safe_ptr_cast<const T*>(Data()) + idx);
      }

      static Ptr Create(const String& filename);
      static Ptr Create(const Dump& data);
    };

    template<class T>
    class FastDump
    {
    public:
      explicit FastDump(const DataContainer& data)
        : Ptr(safe_ptr_cast<const T*>(data.Data())), Lenght(data.Size() / sizeof(T))
      {
        assert(0 == Lenght % sizeof(T));
      }

      const T& operator [] (std::size_t idx) const
      {
        assert(idx < Lenght);
        return Ptr[idx];
      }

      std::size_t Size() const
      {
        return Lenght;
      }
    private:
      const T* const Ptr;
      const std::size_t Lenght;
    };
  }
}

#endif //__CONTAINER_H_DEFINED__
