/**
 *
 * @file
 *
 * @brief  Dump type definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "types.h"  // IWYU pragma: export

#include <vector>  // IWYU pragma: export

namespace Binary
{
  //! @brief Plain data type
  using Dump = std::vector<uint8_t>;
}  // namespace Binary
