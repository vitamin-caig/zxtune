/*
Abstract:
  Format detector implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "detector.h"
#include "iterator.h"
//common includes
#include <types.h>
//std includes
#include <cassert>
#include <cctype>
#include <vector>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const char ANY_BYTE_TEXT = '?';
  const char ANY_NIBBLE_TEXT = 'x';
  const char SKIP_BYTES_TEXT = '+';
  const char BINARY_MASK_TEXT = '%';
  const char SYMBOL_TEXT = '\'';
  const char RANGE_TEXT = '-';

  const char ANY_BIT_TEXT = 'x';
  const char ZERO_BIT_TEXT = '0';
  const char ONE_BIT_TEXT = '1';

  typedef RangeIterator<std::string::const_iterator> PatternIterator;

  inline std::size_t ParseSkipBytes(PatternIterator& it)
  {
    std::size_t skip = 0;
    while (SKIP_BYTES_TEXT != *++it)
    {
      assert((it && std::isdigit(*it)) || !"Invalid pattern format");
      skip = skip * 10 + (*it - '0');
    }
    return skip;
  }

  struct Binary
  {
    typedef std::pair<uint8_t, uint8_t> PatternEntry;
    BOOST_STATIC_ASSERT(sizeof(PatternEntry) == 2);
    typedef std::vector<PatternEntry> Pattern;

    inline static PatternEntry GetAnyByte()
    {
      return PatternEntry(0, 0);
    }

    inline static bool CompareEntryToByte(const PatternEntry& lh, uint8_t rh)
    {
      return lh.first ? ((lh.first & rh) == lh.second) : true;
    }

    inline static bool CompareByteToEntry(uint8_t lh, const PatternEntry& rh)
    {
      return CompareEntryToByte(rh, lh);
    }

    inline static uint8_t NibbleToMask(char c)
    {
      assert(std::isxdigit(c) || ANY_NIBBLE_TEXT == c);
      return ANY_NIBBLE_TEXT == c
        ? 0 : 0xf;
    }

    inline static uint8_t NibbleToValue(char c)
    {
      assert(std::isxdigit(c) || ANY_NIBBLE_TEXT == c);
      return ANY_NIBBLE_TEXT == c
        ? 0 : (std::isdigit(c) ? c - '0' : std::toupper(c) - 'A' + 10);
    }

    inline static PatternEntry ParseNibbles(PatternIterator& it)
    {
      const char hiNibble(*it);
      ++it;
      assert(it || !"Invalid pattern format");
      const char loNibble(*it);
      const uint8_t mask = NibbleToMask(hiNibble) * 16 + NibbleToMask(loNibble);
      const uint8_t value = NibbleToValue(hiNibble) * 16 + NibbleToValue(loNibble);
      return PatternEntry(mask, value);
    }

    inline static PatternEntry ParseBinary(PatternIterator& it)
    {
      uint8_t mask = 0;
      uint8_t value = 0;
      for (uint_t bitmask = 128; bitmask; bitmask >>= 1)
      {
        ++it;
        assert(it || !"Invalid pattern format");
        switch (*it)
        {
        case ONE_BIT_TEXT:
          value |= bitmask;
        case ZERO_BIT_TEXT:
          mask |= bitmask;
          break;
        case ANY_BIT_TEXT:
          break;
        default: 
          assert(!"Invalid pattern format");
          break;
        }
      }
      return PatternEntry(mask, value);
    }

    static void CompilePattern(const std::string& textPattern, Pattern& binPattern)
    {
      Pattern result;
      for (PatternIterator it(textPattern.begin(), textPattern.end()); it; ++it)
      {
        switch (*it)
        {
        case ANY_BYTE_TEXT:
          result.push_back(GetAnyByte());
          break;
        case SKIP_BYTES_TEXT:
          {
            const std::size_t skip = ParseSkipBytes(it);
            std::fill_n(std::back_inserter(result), skip, GetAnyByte());
          }
          break;
        case BINARY_MASK_TEXT:
          {
            const PatternEntry& entry = ParseBinary(it);
            result.push_back(entry);
          }
          break;
        case SYMBOL_TEXT:
          {
            const uint8_t val = static_cast<uint8_t>(*++it);
            const PatternEntry& entry = PatternEntry(0xff, val);
            result.push_back(entry);
          }
          break;
        case ' ':
        case '\n':
        case '\r':
        case '\t':
          break;
        default:
          {
            const PatternEntry& entry = ParseNibbles(it);
            result.push_back(entry);
          }
        }
      }
      binPattern.swap(result);
    }
  };

  struct Ranged
  {
    typedef std::pair<uint8_t, uint8_t> PatternEntry;
    BOOST_STATIC_ASSERT(sizeof(PatternEntry) == 2);
    typedef std::vector<PatternEntry> Pattern;

    inline static PatternEntry GetAnyByte()
    {
      return PatternEntry(0, 255);
    }

    inline static bool CompareEntryToByte(const PatternEntry& lh, uint8_t rh)
    {
      return lh.first <= rh && rh <= lh.second;
    }

    inline static bool CompareByteToEntry(uint8_t lh, const PatternEntry& rh)
    {
      return CompareEntryToByte(rh, lh);
    }

    inline static uint8_t NibbleToValue(char c)
    {
      assert(std::isxdigit(c));
      return (std::isdigit(c) ? c - '0' : std::toupper(c) - 'A' + 10);
    }

    inline static uint8_t ParseNibbles(PatternIterator& it)
    {
      const char hiNibble(*it);
      ++it;
      assert(it || !"Invalid pattern format");
      const char loNibble(*it);
      return NibbleToValue(hiNibble) * 16 + NibbleToValue(loNibble);
    }

    static void CompilePattern(const std::string& textPattern, Pattern& rngPattern)
    {
      Pattern result;
      for (PatternIterator it(textPattern.begin(), textPattern.end()); it; ++it)
      {
        switch (*it)
        {
        case ANY_BYTE_TEXT:
          result.push_back(GetAnyByte());
          break;
        case SKIP_BYTES_TEXT:
          {
            const std::size_t skip = ParseSkipBytes(it);
            std::fill_n(std::back_inserter(result), skip, GetAnyByte());
          }
          break;
        case SYMBOL_TEXT:
          {
            const uint8_t val = static_cast<uint8_t>(*++it);
            const PatternEntry& entry = PatternEntry(val, val);
            result.push_back(entry);
          }
          break;
        case RANGE_TEXT:
          {
            assert(!result.empty() && result.back().first == result.back().second);
            const uint8_t upper = ParseNibbles(++it);
            result.back().second = upper;
          }
          break;
        case ' ':
        case '\n':
        case '\r':
        case '\t':
          break;
        default:
          {
            const uint8_t b = ParseNibbles(it);
            result.push_back(PatternEntry(b, b));
          }
        }
      }
      rngPattern.swap(result);
    }
  };

  template<class Type>
  class Format : public DataFormat
  {
    explicit Format(typename Type::Pattern::const_iterator from, typename Type::Pattern::const_iterator to, std::size_t offset)
      : Pat(from, to)
      , Offset(offset)
    {
    }
  public:
    static DataFormat::Ptr Create(const std::string& pattern)
    {
      typename Type::Pattern pat;
      Type::CompilePattern(pattern, pat);
      const typename Type::Pattern::const_iterator first = pat.begin();
      const typename Type::Pattern::const_iterator last = pat.end();
      const typename Type::Pattern::const_iterator firstNotAny = std::find_if(first, last, 
        std::bind2nd(std::not_equal_to<typename Type::PatternEntry>(), Type::GetAnyByte()));
      const std::size_t offset = std::distance(first, firstNotAny);
      return DataFormat::Ptr(new Format<Type>(firstNotAny, last, offset));
    }

    virtual bool Match(const void* data, std::size_t size) const
    {
      if (Offset + Pat.size() > size)
      {
        return false;
      }
      const uint8_t* const typedData = static_cast<const uint8_t*>(data) + Offset;
      return std::equal(Pat.begin(), Pat.end(), typedData, &Type::CompareEntryToByte);
    }

    virtual std::size_t Search(const void* data, std::size_t size) const
    {
      if (Offset + Pat.size() > size)
      {
        return size;
      }
      const uint8_t* const typedData = static_cast<const uint8_t*>(data) + Offset;
      return std::search(typedData, typedData + size - Offset, Pat.begin(), Pat.end(), &Type::CompareByteToEntry) - typedData;
    }
  private:
    const typename Type::Pattern Pat;
    const std::size_t Offset;
  };
}

DataFormat::Ptr DataFormat::Create(const std::string& pattern)
{
  const bool hasBinaryMasks = pattern.find(BINARY_MASK_TEXT) != std::string::npos ||
                              pattern.find(ANY_NIBBLE_TEXT) != std::string::npos;
  const bool hasRanges = pattern.find(RANGE_TEXT) != std::string::npos;
  if (!hasRanges)
  {
    return Format<Binary>::Create(pattern);
  }
  else if (hasRanges && !hasBinaryMasks)
  {
    return Format<Ranged>::Create(pattern);
  }
  assert(!"Invalid pattern format");
  return DataFormat::Ptr();
}
