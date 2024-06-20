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

namespace Strings
{
  StringView TrimSpaces(StringView str)
  {
    return Trim(str, [](Char c) { return c <= ' '; });
  }
}  // namespace Strings
