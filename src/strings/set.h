/**
 *
 * @file
 *
 * @brief  Simple strings set typedef
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_type.h"  // IWYU pragma: export

#include <set>  // IWYU pragma: export

namespace Strings
{
  using Set = std::set<String, std::less<>>;
}
