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

// common includes
#include <string_type.h>
// std includes
#include <set>

namespace Strings
{
  using Set = std::set<String, std::less<>>;
}
