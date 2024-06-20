/**
 *
 * @file
 *
 * @brief  String split
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <strings/src/find.h>
// std includes
#include <vector>

namespace Strings
{
  template<class D>
  std::vector<StringView> Split(StringView str, D delimiter)
  {
    std::vector<StringView> target;
    auto it = str.substr(Find::FirstNot(str, delimiter));
    do
    {
      const auto part = Find::First(it, delimiter);
      target.emplace_back(it.substr(0, part));
      const auto delim = Find::FirstNot(it.substr(part), delimiter);
      it = it.substr(part + delim);
    } while (!it.empty());
    return target;
  }
}  // namespace Strings
