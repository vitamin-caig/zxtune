/**
 *
 * @file
 *
 * @brief  String optimization
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
#include <types.h>

namespace Strings
{
  class Utf8Builder
  {
  public:
    Utf8Builder() = default;

    template<class It>
    Utf8Builder(It begin, It end)
    {
      Reserve(end - begin);
      for (auto it = begin; it != end; ++it)
      {
        Add(*it);
      }
    }

    void Reserve(std::size_t symbols)
    {
      Result.reserve(symbols);
    }

    void Add(uint32_t sym)
    {
      if (sym < 0x80)
      {
        AddRaw(sym);
      }
      else if (sym < 0x800)
      {
        // bbbbbaaaaaa
        // 110bbbbb 10aaaaaa
        AddRaw(0xc0 | (sym >> 6));
        AddSeq(sym >> 0);
      }
      else if (sym < 0x10000)
      {
        // aaaabbbbbbcccccc
        // 1110aaaa 10bbbbbb 10cccccc
        AddRaw(0xe0 | (sym >> 12));
        AddSeq(sym >> 6);
        AddSeq(sym >> 0);
      }
      else if (sym < 0x110000)
      {
        // aaabbbbbbccccccdddddd
        // 11110aaa 10bbbbbb 10cccccc 10dddddd
        AddRaw(0xf0 | (sym >> 18));
        AddSeq(sym >> 12);
        AddSeq(sym >> 6);
        AddSeq(sym >> 0);
      }
      else
      {
        AddRaw('?');
      }
    }

    String GetResult()
    {
      return std::move(Result);
    }

  private:
    void AddSeq(uint32_t val)
    {
      AddRaw(0x80 | (val & 0x3f));
    }

    void AddRaw(uint8_t val)
    {
      Result += static_cast<String::value_type>(val);
    }

  private:
    String Result;
  };

  inline bool IsUtf8(StringView str)
  {
    // https://en.wikipedia.org/wiki/UTF-8#Description
    for (auto it = str.begin(), lim = str.end(); it != lim;)
    {
      //%0xxxxxx
      const uint_t sym = *it;
      ++it;
      if (0 == (sym & 0x80))
      {
        continue;
      }
      uint_t restBytes = 0;
      for (uint_t mask = 0x40; 0 != (sym & mask); ++restBytes, mask >>= 1)
      {
        if (restBytes > 3)
        {
          return false;
        }
      }
      if (!restBytes || it + restBytes > lim)
      {
        return false;
      }
      for (; restBytes != 0; --restBytes, ++it)
      {
        if (0x80 != (uint8_t(*it) & 0xc0))
        {
          return false;
        }
      }
    }
    return true;
  }

  inline StringView CutUtf8BOM(StringView str)
  {
    if (str.size() >= 3 && str[0] == 0xef && str[1] == 0xbb && str[2] == 0xbf)
    {
      return str.substr(3);
    }
    else
    {
      return str;
    }
  }

  inline bool IsUtf16(StringView str)
  {
    return str.size() > 2 && str.size() % 2 == 0
           && ((str[0] == 0xff && str[1] == 0xfe) || (str[0] == 0xfe && str[1] == 0xff));
  }
}  // namespace Strings
