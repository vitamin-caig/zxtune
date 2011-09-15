/**
*
* @file      io/container.h
* @brief     Data container interface and related
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __IO_CONTAINER_H_DEFINED__
#define __IO_CONTAINER_H_DEFINED__

//library includes
#include <binary/container.h>

namespace ZXTune
{
  namespace IO
  {
    //! @brief Fast std::vector-alike wrapper around DataContainer
    class FastDump
    {
    public:
      FastDump(const Binary::Container& data, std::size_t offset = 0, std::size_t lenght = ~0)
        : Ptr(static_cast<const uint8_t*>(data.Data()) + offset)
        , Lenght(std::min(lenght, data.Size() - offset))
      {
      }

      FastDump(const void* data, std::size_t size)
        : Ptr(static_cast<const uint8_t*>(data))
        , Lenght(size)
      {
      }

      //! @brief Accessing elements by index
      const uint8_t& operator [] (std::size_t idx) const
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
      const uint8_t* Data() const
      {
        return Ptr;
      }
    private:
      const uint8_t* const Ptr;
      const std::size_t Lenght;
    };
  }
}

#endif //__IO_CONTAINER_H_DEFINED__
