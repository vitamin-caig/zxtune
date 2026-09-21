/**
 *
 * @file
 *
 * @brief  Packed data support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"

#include <memory>

namespace Formats::Packed
{
  //! @brief Unpacked data
  class Container : public Binary::Container
  {
  public:
    using Ptr = std::shared_ptr<const Container>;

    //! @brief Getting size of source data this container was unpacked from
    //! @return Size in bytes
    //! @invariant Result is always > 0
    virtual std::size_t PackedSize() const = 0;
  };
}  // namespace Formats::Packed
