/**
 *
 * @file
 *
 * @brief  Casing-related
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common included
#include <string_view.h>
#include <types.h>

namespace Strings
{
  String ToLowerAscii(StringView str);
  String ToUpperAscii(StringView str);

  bool EqualNoCaseAscii(StringView lh, StringView rh);
}  // namespace Strings
