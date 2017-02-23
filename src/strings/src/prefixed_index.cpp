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
  PrefixedIndex::PrefixedIndex(StringView prefix, StringView value)
    : Str(value.to_string())
    , Valid(false)
    , Index()
  {
    if (0 == value.compare(0, prefix.size(), prefix))
    {
      std::basic_istringstream<Char> str(Str.substr(prefix.size()));
      str >> Index;
      Valid = !!str;
    }
  }

  PrefixedIndex::PrefixedIndex(StringView prefix, uint_t index)
    : Str(prefix.to_string() + std::to_string(index))
    , Valid(true)
    , Index(index)
  {
  }
}
