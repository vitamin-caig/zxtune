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
// std includes
#include <algorithm>
// boost includes
#include <boost/algorithm/string/trim.hpp>

namespace Strings
{
  const auto IsAsciiNoSpace = boost::is_from_range('\x21', '\x7e');
  const auto IsAscii = boost::is_from_range('\x20', '\x7e');

  String OptimizeAscii(StringView str, Char replacement)
  {
    auto res = String{boost::algorithm::trim_copy_if(StringViewCompat{str.begin(), str.end()}, !IsAsciiNoSpace)};
    std::replace_if(res.begin(), res.end(), !IsAscii, replacement);
    return res;
  }
}  // namespace Strings
