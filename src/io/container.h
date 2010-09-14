/**
*
* @file      io/container.h
* @brief     Data container interface and related
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __IO_CONTAINER_H_DEFINED__
#define __IO_CONTAINER_H_DEFINED__

//common includes
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  namespace IO
  {
    //! @brief Data container abstraction interface
    class DataContainer
    {
    public:
      //! @brief Pointer type
      typedef boost::shared_ptr<DataContainer> Ptr;

      virtual ~DataContainer() {}

      //! @brief Getting data size in bytes
      virtual std::size_t Size() const = 0;
      //! @brief Getting raw data accessible for at least Size() bytes
      virtual const void* Data() const = 0;
      //! @brief Provides isolated access to nested subcontainers should be able even after parent container destruction
      virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const = 0;
    };

    //! @brief Creating data container based on raw data
    DataContainer::Ptr CreateDataContainer(const Dump& data);
    DataContainer::Ptr CreateDataContainer(const void* data, std::size_t size);

    //! @brief Fast std::vector-alike wrapper around DataContainer
    class FastDump
    {
    public:
      FastDump(const DataContainer& data, std::size_t offset = 0)
        : Ptr(static_cast<const unsigned char*>(data.Data()) + offset)
        , Lenght(data.Size() - offset)
      {
      }

      //! @brief Accessing elements by index
      const unsigned char& operator [] (std::size_t idx) const
      {
        assert(idx < Lenght);
        return Ptr[idx];
      }

      //! @brief Getting size
      std::size_t Size() const
      {
        return Lenght;
      }
      
      //! @brief Getting raw data pointer
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
