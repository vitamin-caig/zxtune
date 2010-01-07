/*
Abstract:
  Data container interface and related

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __IO_CONTAINER_H_DEFINED__
#define __IO_CONTAINER_H_DEFINED__

#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  namespace IO
  {
    class DataContainer
    {
    public:
      typedef boost::shared_ptr<DataContainer> Ptr;
      
      virtual ~DataContainer()
      {
      }

      //size in bytes
      virtual std::size_t Size() const = 0;
      //raw data accessible for at least Size() bytes
      virtual const void* Data() const = 0;
      //isolated access to nested subcontainers should be able even after parent container destruction
      virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const = 0;
    };

    //fast std::vector-alike wrapper around DataContainer
    class FastDump
    {
    public:
      FastDump(const DataContainer& data, std::size_t offset = 0)
        : Ptr(static_cast<const unsigned char*>(data.Data()) + offset)
        , Lenght(data.Size() - offset)
      {
      }

      const unsigned char& operator [] (std::size_t idx) const
      {
        assert(idx < Lenght);
        return Ptr[idx];
      }

      std::size_t Size() const
      {
        return Lenght;
      }
      
      const unsigned char* Data() const
      {
        return Ptr;
      }
    private:
      const unsigned char* const Ptr;
      const std::size_t Lenght;
    };
  }
}

#endif //__IO_CONTAINER_H_DEFINED__
