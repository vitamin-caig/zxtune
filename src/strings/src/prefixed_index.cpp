/**
 *
 * @file
 *
 * @brief  Simple parser for <prefix><index> pairs implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <strings/conversion.h>
#include <strings/prefixed_index.h>

namespace Strings
{
  PrefixedIndex::PrefixedIndex(StringView prefix, StringView value)
    : Str(value.to_string())
    , Valid(false)
    , Index()
  {
    if (0 == value.compare(0, prefix.size(), prefix))
    {
      Valid = Parse<decltype(Index)>(value.substr(prefix.size()), Index);
    }
  }

  PrefixedIndex::PrefixedIndex(StringView prefix, std::size_t index)
    : Str(prefix.to_string() + ConvertFrom(index))
    , Valid(true)
    , Index(index)
  {}
}  // namespace Strings
