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

#include "string_type.h"

#include <set>

namespace Strings
{
  using Set = std::set<String, std::less<>>;
}
