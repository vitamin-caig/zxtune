/**
 *
 * @file
 *
 * @brief  Compile-time string
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <array>
#include <string_view>

template<typename C, C... Chars>
struct basic_static_string
{
  constexpr operator std::basic_string_view<C>() const
  {
    return {data.data(), data.size()};
  }

private:
  constexpr static const std::array<C, sizeof...(Chars)> data = {Chars...};
};

template<typename C, C... Chars>
constexpr auto operator"" _ss()
{
  return basic_static_string<C, Chars...>{};
}

template<typename C, C... Chars1, C... Chars2>
constexpr auto operator+(const basic_static_string<C, Chars1...>&, const basic_static_string<C, Chars2...>&)
{
  return basic_static_string<C, Chars1..., Chars2...>{};
}
