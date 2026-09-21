/**
 *
 * @file
 *
 * @brief  Images support interfaces
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"

#include <memory>

namespace Formats::Image
{
  //! @brief Image raw data presentation
  class Container : public Binary::Container
  {
  public:
    using Ptr = std::shared_ptr<const Container>;

    //! @brief Getting size of source data this container was extracted from
    //! @return Size in bytes
    //! @invariant Result is always > 0
    virtual std::size_t OriginalSize() const = 0;
  };
}  // namespace Formats::Image
