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

using std::string_view_literals::operator""sv;

template<class Array>
constexpr auto MakeStringView(const Array& array) noexcept
{
  return std::basic_string_view<typename Array::value_type>(array.data(), array.size());
}

// Workaround for Clang12 (Android)
template<class It, class End>
auto MakeStringView(It first, End last) noexcept
{
  const auto* start = std::to_address(first);
  const std::size_t size = last - first;
  return std::basic_string_view<std::remove_cvref_t<decltype(*start)>>(start, size);
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
