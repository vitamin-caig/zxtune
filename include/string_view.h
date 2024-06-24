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
#include <memory>
#include <string>
#include <string_view>

namespace std
{
  // Workaround for Clang12 (Android)
  template<class C>
  class basic_string_view_compat : public basic_string_view<C>
  {
  public:
    template<class It, class End>
    basic_string_view_compat(It first, End last)
      : basic_string_view<C>(std::to_address(first), static_cast<std::size_t>(last - first))
    {}
  };
}  // namespace std

constexpr auto operator"" _sv(const char* str, std::size_t size) noexcept
{
  return std::basic_string_view<char>{str, size};
}

template<class Array>
constexpr auto MakeStringView(const Array& array) noexcept
{
  return std::basic_string_view<typename Array::value_type>(array.data(), array.size());
}

// TODO: Remove after c++26
template<class Char>
std::basic_string<Char> operator+(const std::basic_string<Char>& lhs, std::basic_string_view<Char> rhs)
{
  std::basic_string<Char> ret = lhs;
  ret.append(rhs);
  return ret;
}

template<class Char>
std::basic_string<Char> operator+(std::basic_string<Char>&& lhs, std::basic_string_view<Char> rhs)
{
  lhs.append(rhs);
  return std::move(lhs);
}

template<class Char>
std::basic_string<Char> operator+(std::basic_string_view<Char> lhs, const std::basic_string<Char>& rhs)
{
  std::basic_string<Char> ret = rhs;
  ret.insert(0, lhs);
  return ret;
}
template<class Char>
std::basic_string<Char> operator+(std::basic_string_view<Char> lhs, std::basic_string<Char>&& rhs)
{
  rhs.insert(0, lhs);
  return std::move(rhs);
}
