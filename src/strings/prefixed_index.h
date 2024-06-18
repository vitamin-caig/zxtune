/**
 *
 * @file
 *
 * @brief  Simple parser for <prefix><index> pairs
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>

namespace Strings
{
  class PrefixedIndex
  {
  public:
    static PrefixedIndex Parse(StringView prefix, StringView value);
    static PrefixedIndex ParseNoCase(StringView prefix, StringView value);
    static PrefixedIndex Create(StringView prefix, std::size_t index);

    bool IsValid() const
    {
      return Valid;
    }

    std::size_t GetIndex() const
    {
      return Index;
    }

    String ToString() const
    {
      return String{Str};
    }

  private:
    PrefixedIndex(String str, bool isValid, std::size_t idx)
      : Str(std::move(str))
      , Valid(isValid)
      , Index(idx)
    {}

  private:
    const String Str;
    const bool Valid;
    const std::size_t Index;
  };
}  // namespace Strings
