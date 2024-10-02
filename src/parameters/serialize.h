/**
 *
 * @file
 *
 * @brief  Serialize API
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
// library includes
#include <strings/map.h>

namespace Parameters
{
  void Convert(StringView name, StringView value, class Visitor& visitor);
  void Convert(const Strings::Map& map, class Visitor& visitor);
  void Convert(const class Accessor& ac, Strings::Map& strings);
}  // namespace Parameters
