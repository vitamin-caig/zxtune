/**
*
* @file      binary/typed_container.h
* @brief     Typed container helper
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef BINARY_TYPED_CONTAINER_H_DEFINED
#define BINARY_TYPED_CONTAINER_H_DEFINED

//library includes
#include <binary/data.h>
//common includes
#include <types.h>
#include <pointers.h>

namespace Binary
{
  //! @brief Simple data access wrapper around Binary::Container with limitation support
  class TypedContainer
  {
  public:
    TypedContainer(const Data& delegate, std::size_t size = ~std::size_t(0))
      : Start(static_cast<const uint8_t*>(delegate.Start()))
      , Size(std::min(delegate.Size(), size))
    {
    }

    //! @brief Getting field with specfieid type on specified offset
    //! @param offset Offset of field in bytes
    //! @return 0 if data cannot be accessed (no place)
    template<class T>
    const T* GetField(std::size_t offset) const
    {
      return offset + sizeof(T) <= Size
        ? safe_ptr_cast<const T*>(Start + offset)
        : 0;
    }

    std::size_t GetSize() const
    {
      return Size;
    }
  private:
    const uint8_t* const Start;
    const std::size_t Size;
  };
}

#endif //__BINARY_CONTAINER_H_DEFINED__
