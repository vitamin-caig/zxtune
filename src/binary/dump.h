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

// common includes
#include <types.h>
// std includes
#include <vector>

namespace Binary
{
  //! @brief Plain data type
  using Dump = std::vector<uint8_t>;
}  // namespace Binary
