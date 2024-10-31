/**
 *
 * @file
 *
 * @brief  String optimization
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_type.h"  // IWYU pragma: export
#include "string_view.h"  // IWYU pragma: export

#include <array>
#include <cstddef>

namespace Strings
{
  // trim all the non-ascii symbols from begin/end and replace by replacement at the middle
  String OptimizeAscii(StringView str, char replacement);

  inline String OptimizeAscii(StringView str)
  {
    return OptimizeAscii(str, '\?');
  }

  template<class Char, std::size_t D>
  inline auto OptimizeAscii(const std::array<Char, D>& data, Char replacement = '\?')
  {
    return OptimizeAscii(MakeStringView(data), replacement);
  }
}  // namespace Strings
