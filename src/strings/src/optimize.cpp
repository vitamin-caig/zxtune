/**
 *
 * @file
 *
 * @brief  String optimization implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <strings/optimize.h>
#include <strings/trim.h>
// std includes
#include <algorithm>

namespace Strings
{
  String OptimizeAscii(StringView str, Char replacement)
  {
    static constexpr auto isWhitespace = [](Char c) { return c <= ' ' || c > '\x7e'; };
    String res{Trim(str, isWhitespace)};
    std::replace_if(
        res.begin(), res.end(), [](Char c) { return c != ' ' && isWhitespace(c); }, replacement);
    return res;
  }
}  // namespace Strings
