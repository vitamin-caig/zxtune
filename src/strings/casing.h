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

#include "string_type.h"  // IWYU pragma: export
#include "string_view.h"  // IWYU pragma: export

namespace Strings
{
  String ToLowerAscii(StringView str);
  String ToUpperAscii(StringView str);

  bool EqualNoCaseAscii(StringView lh, StringView rh);
}  // namespace Strings
