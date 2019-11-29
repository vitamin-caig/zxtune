/**
*
* @file
*
* @brief  Typed container helper
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/view.h>
//common includes
#include <types.h>
#include <pointers.h>
//std includes
#include <algorithm>

namespace Binary
{
  //! @brief Simple data access wrapper around Binary::Container with limitation support
  class TypedContainer
  {
  public:
    TypedContainer(View delegate, std::size_t size = ~std::size_t(0))
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
      return offset < Size && offset + sizeof(T) <= Size
        ? safe_ptr_cast<const T*>(Start + offset)
        : nullptr;
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
