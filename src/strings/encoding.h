/**
 *
 * @file
 *
 * @brief  Encoding-related
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common included
#include <string_type.h>
#include <string_view.h>

namespace Strings
{
  String ToAutoUtf8(StringView str);

  String Utf16ToUtf8(std::basic_string_view<uint16_t> str);
}  // namespace Strings
