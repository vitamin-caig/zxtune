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
// boost includes
#include <boost/algorithm/string/trim.hpp>

namespace Strings
{
  StringView TrimSpaces(StringView str)
  {
    return boost::algorithm::trim_copy_if(StringViewCompat{str.begin(), str.end()},
                                          boost::is_from_range('\x00', '\x20'));
  }
}  // namespace Strings
