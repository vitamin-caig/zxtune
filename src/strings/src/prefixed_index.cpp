/**
*
* @file
*
* @brief  Simple parser for <prefix><index> pairs implementation
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <strings/prefixed_index.h>
//std includes
#include <sstream>

namespace Strings
{
  PrefixedIndex::PrefixedIndex(const String& prefix, const String& value)
    : Str(value)
    , Valid(false)
    , Index()
  {
    if (0 == value.compare(0, prefix.size(), prefix))
    {
      std::basic_istringstream<Char> str(value.substr(prefix.size()));
      str >> Index;
      Valid = !!str;
    }
  }

  PrefixedIndex::PrefixedIndex(const String& prefix, uint_t index)
    : Valid(true)
    , Index(index)
  {
    std::basic_ostringstream<Char> str;
    str << prefix << index;
    Str = str.str();
  }
}
