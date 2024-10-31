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

#include <strings/map.h>

#include <string_view.h>

namespace Parameters
{
  void Convert(StringView name, StringView value, class Visitor& visitor);
  void Convert(const Strings::Map& map, class Visitor& visitor);
  void Convert(const class Accessor& ac, Strings::Map& strings);
}  // namespace Parameters
