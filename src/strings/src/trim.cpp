/**
 *
 * @file
 *
 * @brief  String trimming implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <strings/trim.h>
// common includes
#include <string_view.h>

namespace Strings
{
  StringView TrimSpaces(StringView str)
  {
    return Trim(str, [](auto c) { return c <= ' '; });
  }
}  // namespace Strings
