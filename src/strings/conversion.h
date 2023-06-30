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
#include <algorithm>
#include <limits>
#include <type_traits>

// gcc4.8 for android toolchain does not have std::to_string/std::stoi/etc
namespace Strings
{
  namespace Details
  {
    template<class T>
    struct ConversionTraits;

    template<class T>
    struct ConversionTraits
    {
      static_assert(std::is_integral<T>::value, "TODO");

      static T Parse(StringView& str)
      {
        if (str.empty())
        {
          return T();
        }
        else
        {
          T result = T();
          const auto* it = str.begin();
          const auto* const lim = str.end();
          const bool hasMinus = std::is_signed<T>::value && *it == '-';
          if (*it == '-' || *it == '+')
          {
            ++it;
          }
          for (; it != lim; ++it)
          {
            const auto sym = *it;
            if (sym >= '0' && sym <= '9')
            {
              // TODO: handle overflow
              result = result * 10 + static_cast<T>(sym - '0');
            }
            else
            {
              break;
            }
          }
          str = StringView(it, lim);
          return hasMinus ? -result : result;
        }
      }
    };
  }  // namespace Details

  template<class T>
  inline T ParsePartial(StringView& str)
  {
    return Details::ConversionTraits<T>::Parse(str);
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
