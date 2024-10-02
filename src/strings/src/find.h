/**
 *
 * @file
 *
 * @brief  String match helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>
// std includes
#include <algorithm>

namespace Strings
{
  namespace Find
  {
    template<class T>
    concept CharPredicate = requires(T f, Char c)
    {
      f(c);
    };

    template<CharPredicate Pred>
    constexpr std::size_t First(StringView str, Pred pred)
    {
      return std::find_if(str.begin(), str.end(), pred) - str.begin();
    }

    template<class Match>
    constexpr std::size_t First(StringView str, Match match)
    {
      const auto pos = str.find_first_of(match);
      return pos != StringView::npos ? pos : str.size();
    }

    template<CharPredicate Pred>
    constexpr std::size_t FirstNot(StringView str, Pred pred)
    {
      return std::find_if_not(str.begin(), str.end(), pred) - str.begin();
    }

    template<class Match>
    constexpr std::size_t FirstNot(StringView str, Match match)
    {
      const auto pos = str.find_first_not_of(match);
      return pos != StringView::npos ? pos : str.size();
    }

    // Returns -1 if nothing found or index
    template<CharPredicate Pred>
    constexpr std::ptrdiff_t Last(StringView str, Pred pred)
    {
      const auto it = std::find_if(str.rbegin(), str.rend(), pred);
      return it.base() - str.begin() - 1;
    }

    template<class Match>
    constexpr std::ptrdiff_t Last(StringView str, Match match)
    {
      const auto pos = str.find_last_of(match);
      return pos != StringView::npos ? pos : -1;
    }

    template<CharPredicate Pred>
    constexpr std::ptrdiff_t LastNot(StringView str, Pred pred)
    {
      const auto it = std::find_if_not(str.rbegin(), str.rend(), pred);
      return it.base() - str.begin() - 1;
    }

    template<class Match>
    constexpr std::ptrdiff_t LastNot(StringView str, Match match)
    {
      const auto pos = str.find_last_not_of(match);
      return pos != StringView::npos ? pos : -1;
    }
  }  // namespace Find
}  // namespace Strings
