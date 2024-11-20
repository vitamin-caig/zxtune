/**
 *
 * @file
 *
 * @brief  String sanitize functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_type.h"  // IWYU pragma: export
#include "string_view.h"  // IWYU pragma: export

namespace Strings
{
  /*           | \t  | \r,\n,\r\n | ctrl | trim?
  Sanitize     | ' ' | ' '        | none | yes
  *Multiline   | ' ' | \n         | none | every line
  *KeepPadding | ' ' | ' '        | ' '  | right only
  */
  String Sanitize(StringView str);
  String SanitizeMultiline(StringView str);
  String SanitizeKeepPadding(StringView str);
}  // namespace Strings
