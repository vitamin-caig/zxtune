/**
 *
 * @file
 *
 * @brief  String trimming
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "strings/src/find.h"

#include "string_view.h"

namespace Strings
{
  template<class P>
  StringView Trim(StringView str, P toTrim)
  {
    const auto start = Find::FirstNot(str, toTrim);
    const auto end = Find::LastNot(str, toTrim);
    return str.substr(start, end + 1 - start);
  }

  StringView TrimSpaces(StringView str);
}  // namespace Strings
