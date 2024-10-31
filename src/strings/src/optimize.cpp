/**
 *
 * @file
 *
 * @brief  String optimization implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <strings/optimize.h>
#include <strings/trim.h>

#include <string_view.h>

#include <algorithm>

namespace Strings
{
  String OptimizeAscii(StringView str, char replacement)
  {
    static constexpr auto isWhitespace = [](char c) { return c <= ' ' || c > '\x7e'; };
    String res{Trim(str, isWhitespace)};
    std::replace_if(
        res.begin(), res.end(), [](auto c) { return c != ' ' && isWhitespace(c); }, replacement);
    return res;
  }
}  // namespace Strings
