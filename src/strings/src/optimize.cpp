/**
*
* @file
*
* @brief  String optimization implementation
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <strings/optimize.h>
//std includes
#include <algorithm>
#include <cctype>
//boost includes
#include <boost/algorithm/string/trim.hpp>

namespace Strings
{
  String Optimize(const String& str)
  {
    String res(boost::algorithm::trim_copy_if(str, !boost::is_from_range('\x21', '\x7f')));
    std::replace_if(res.begin(), res.end(), std::ptr_fun<int, int>(&std::iscntrl), '\?');
    return res;
  }
}
