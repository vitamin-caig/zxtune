/**
 *
 * @file
 *
 * @brief  Binary data container interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/data.h"  // IWYU pragma: export

#include <cstddef>
#include <memory>

namespace Binary
{
  class Container : public Data
  {
  public:
    //! @brief Pointer type
    using Ptr = std::shared_ptr<const Container>;

    //! @brief Provides isolated access to nested subcontainers should be able even after parent container destruction
    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const = 0;
  };
}  // namespace Binary
