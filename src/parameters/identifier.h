/**
 *
 * @file
 *
 * @brief  Namespace-typed identifiers support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>

namespace Parameters
{
  template<typename C, C... Symbols>
  class Identifier
  {
  public:
    constexpr operator basic_string_view<C>() const
    {
      return {Storage.data(), Storage.size()};
    }

    template<C... AnotherSymbols>
    constexpr auto operator+(const Identifier<C, AnotherSymbols...>&) const
    {
      static_assert(sizeof...(Symbols) * sizeof...(AnotherSymbols) != 0, "Empty components");
      return Identifier<C, Symbols..., NAMESPACE_DELIMITER, AnotherSymbols...>{};
    }

    constexpr bool IsEmpty() const
    {
      return sizeof...(Symbols) == 0;
    }

    constexpr bool IsPath() const
    {
      constexpr const basic_string_view<C> view{Storage.data(), Storage.size()};
      return view.npos != view.find(NAMESPACE_DELIMITER);
    }

    constexpr basic_string_view<C> Name() const
    {
      constexpr const basic_string_view<C> view{Storage.data(), Storage.size()};
      constexpr const auto lastDelim = view.find_last_of(NAMESPACE_DELIMITER);
      return lastDelim != view.npos ? view.substr(lastDelim + 1) : view;
    }

  private:
    constexpr static const C NAMESPACE_DELIMITER = '.';
    constexpr static const std::array<C, sizeof...(Symbols)> Storage = {Symbols...};
  };

  template<typename C, C... Symbols>
  constexpr auto operator"" _id()
  {
    return Identifier<C, Symbols...>{};
  }
}  // namespace Parameters
