/**
 *
 * @file
 *
 * @brief  Types conversion utilities
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// std includes
#include <charconv>
#include <type_traits>

namespace Strings
{
  template<class T>
  inline T ParsePartial(StringView& str)
  {
    if (str.empty())
    {
      return T();
    }
    else
    {
      auto result = T();
      const auto* const it = str.begin();
      const auto* const lim = str.end();
      const auto res = std::from_chars(it + (it != lim && *it == '+'), lim, result);
      if (res.ec == std::errc::result_out_of_range)
      {
        using WiderType = std::conditional_t<std::is_signed_v<T>, std::intmax_t, std::uintmax_t>;
        return static_cast<T>(ParsePartial<WiderType>(str));
      }
      str = MakeStringView(res.ptr, lim);
      return result;
    }
  }

  // Transactional parsing
  template<class T>
  inline bool Parse(StringView str, T& value)
  {
    if (str.empty())
    {
      return false;
    }
    else
    {
      const auto res = ParsePartial<T>(str);
      if (str.empty())
      {
        value = res;
        return true;
      }
      else
      {
        return false;
      }
    }
  }

  template<class T>
  inline T ConvertTo(StringView str)
  {
    return ParsePartial<T>(str);
  }

  template<class T>
  inline String ConvertFrom(T val)
  {
    return std::to_string(val);
  }
}  // namespace Strings
