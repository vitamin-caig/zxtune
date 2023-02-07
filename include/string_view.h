/**
 *
 * @file
 *
 * @brief  basic_string_view compatibility
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <array>
#include <string>
#include <string_view>

template<class C>
class basic_string_view : public std::basic_string_view<C>
{
  using parent = std::basic_string_view<C>;

  constexpr basic_string_view(const std::basic_string_view<C>& p) noexcept
    : parent(p)
  {}

public:
  constexpr basic_string_view() = default;

  // std::basic_string::operator std::basic_string_view is unusable here
  constexpr basic_string_view(const std::basic_string<C>& str)
    : parent(str.data(), str.size())
  {}

  constexpr basic_string_view(const C* str)
    : parent(str)
  {}

  constexpr basic_string_view(const C* begin, std::size_t size)
    : parent(begin, size)
  {}

  constexpr basic_string_view(const C* begin, const C* end)
    : parent(begin, end - begin)
  {}

  template<std::size_t D>
  constexpr basic_string_view(const std::array<C, D>& str)
    : parent(str.data(), str.size())
  {}

  constexpr basic_string_view<C> substr(std::size_t pos = 0, std::size_t count = parent::npos) const
  {
    return basic_string_view<C>(parent::substr(pos, count));
  }

  // not in all compilers
  constexpr bool starts_with(std::basic_string_view<C> sv) const noexcept
  {
    return parent::size() >= sv.size() && 0 == parent::compare(0, sv.size(), sv);
  }

  // non-standard - ctor is explicit
  constexpr std::basic_string<C> to_string() const
  {
    return std::basic_string<C>(*this);
  }
};

constexpr auto operator"" _sv(const char* str, std::size_t size) noexcept
{
  return basic_string_view<char>{str, size};
}
