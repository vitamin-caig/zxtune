/**
 *
 * @file
 *
 * @brief  String joining
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_type.h"
#include "string_view.h"

#include <numeric>

namespace Strings
{
  template<class Container>
  String Join(const Container& strings, StringView delimiter)
  {
    String result;
    if (strings.empty())
    {
      return result;
    }
    result.reserve(
        std::accumulate(strings.begin(), strings.end(), delimiter.size() * (strings.size() - 1),
                        [](std::size_t sum, typename Container::const_reference str) { return sum + str.size(); }));
    for (const auto& str : strings)
    {
      if (!result.empty())
      {
        result.append(delimiter);
      }
      result.append(str);
    }
    return result;
  }
}  // namespace Strings
