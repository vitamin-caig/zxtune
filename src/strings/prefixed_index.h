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

    PrefixedIndex(StringView prefix, uint_t index);

    bool IsValid() const
    {
      return Valid;
    }

    uint_t GetIndex() const
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
    uint_t Index;
  };
}
