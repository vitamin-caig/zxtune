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
    PrefixedIndex(const String& prefix, const String& value);

    PrefixedIndex(const String& prefix, uint_t index);

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
    String Str;
    bool Valid;
    uint_t Index;
  };
}
