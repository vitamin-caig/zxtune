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

// common includes
#include <string_type.h>
#include <string_view.h>

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
