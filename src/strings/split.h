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

// common includes
#include <types.h>
// std includes
#include <algorithm>
#include <functional>

namespace Strings
{
  namespace Details
  {
    template<bool Matching, class Pred>
    requires(std::is_invocable_v<Pred, Char>) constexpr std::size_t Partition(StringView str, Pred pred)
    {
      const auto it = Matching ? std::find_if_not(str.begin(), str.end(), pred)
                               : std::find_if(str.begin(), str.end(), pred);
      return it - str.begin();
    }

    template<bool Matching, class Match>
    constexpr std::size_t Partition(StringView str, Match match)
    {
      const auto pos = Matching ? str.find_first_not_of(match) : str.find_first_of(match);
      return pos != StringView::npos ? pos : str.size();
    }
  }  // namespace Details

  template<class D>
  std::vector<StringView> Split(StringView str, D delimiter)
  {
    std::vector<StringView> target;
    auto it = str.substr(Details::Partition<true>(str, delimiter));
    do
    {
      const auto part = Details::Partition<false>(it, delimiter);
      target.emplace_back(it.substr(0, part));
      const auto delim = Details::Partition<true>(it.substr(part), delimiter);
      it = it.substr(part + delim);
    } while (!it.empty());
    return target;
  }
}  // namespace Strings
