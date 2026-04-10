/**
 *
 * @file
 *
 * @brief  Format dispatching function
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/format_factories.h"

namespace Binary
{
  Format::Ptr CreateFormat(StringView pattern, std::size_t minSize)
  {
    return CreateMatchOnlyFormat(pattern, minSize);
  }
}  // namespace Binary
