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

//common includes
#include <types.h>

namespace Strings
{
  class PrefixedIndex
  {
  public:
    PrefixedIndex(StringView prefix, StringView value);

    PrefixedIndex(StringView prefix, std::size_t index);

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
      return Str;
    }
  private:
    const String Str;
    bool Valid;
    std::size_t Index;
  };
}
