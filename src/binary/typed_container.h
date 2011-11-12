/**
*
* @file      binary/typed_container.h
* @brief     Typed container helper
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __BINARY_TYPED_CONTAINER_H_DEFINED__
#define __BINARY_TYPED_CONTAINER_H_DEFINED__

//library includes
#include <binary/container.h>
//common includes
#include <tools.h>

namespace Binary
{
  class TypedContainer
  {
  public:
    TypedContainer(const Container& delegate, std::size_t size = ~std::size_t(0))
      : Data(static_cast<const uint8_t*>(delegate.Data()))
      , Size(std::min(delegate.Size(), size))
    {
    }

    template<class T>
    const T* GetField(std::size_t offset) const
    {
      return offset + sizeof(T) <= Size
        ? safe_ptr_cast<const T*>(Data + offset)
        : 0;
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
  };
}

#endif //__BINARY_CONTAINER_H_DEFINED__
