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
//boost includes
#include <boost/array.hpp>
//std includes
#include <bitset>
#include <cassert>
#include <cctype>
#include <stack>
#include <stdexcept>
#include <vector>

namespace
{
  const char ANY_BYTE_TEXT = '?';
  const char ANY_NIBBLE_TEXT = 'x';
  const char SKIP_BYTES_TEXT = '+';
  const char BINARY_MASK_TEXT = '%';
  const char SYMBOL_TEXT = '\'';
  const char RANGE_TEXT = '-';
  const char QUANTOR_BEGIN = '{';
  const char QUANTOR_END = '}';
  const char GROUP_BEGIN = '(';
  const char GROUP_END = ')';

  const char ANY_BIT_TEXT = 'x';
  const char ZERO_BIT_TEXT = '0';
  const char ONE_BIT_TEXT = '1';

  typedef RangeIterator<std::string::const_iterator> PatternIterator;

  void Check(const PatternIterator& it)
  {
    if (!it)
    {
      throw std::out_of_range("Unexpected end of text pattern");
    }
  }

  void CheckParam(bool cond, const char* msg)
  {
    if (!cond)
    {
      throw std::invalid_argument(msg);
    }
  }

  inline std::size_t ParseSkipBytes(PatternIterator& it)
  {
    std::size_t skip = 0;
    while (SKIP_BYTES_TEXT != *++it)
    {
      Check(it);
      CheckParam(0 != std::isdigit(*it), "Skip bytes pattern supports only decimal numbers");
      skip = skip * 10 + (*it - '0');
    }
    return skip;
  }

  inline std::size_t ParseQuantor(PatternIterator& it)
  {
    std::size_t mult = 0;
    while (QUANTOR_END != *++it)
    {
      Check(it);
      CheckParam(0 != std::isdigit(*it), "Quantor supports only decimal numbers");
      mult = mult * 10 + (*it - '0');
    }
    return mult;
  }

  struct BinaryTraits
  {
    typedef std::pair<uint8_t, uint8_t> PatternEntry;
    BOOST_STATIC_ASSERT(sizeof(PatternEntry) == 2);
    typedef std::vector<PatternEntry> Pattern;

    inline static PatternEntry GetAnyByte()
    {
      return PatternEntry(0, 0);
    }

    inline static bool Match(const PatternEntry& lh, uint8_t rh)
    {
      return lh.first ? ((lh.first & rh) == lh.second) : true;
    }

    inline static PatternEntry ParseNibbles(PatternIterator& it)
    {
      Check(it);
      const char hiNibble(*it);
      ++it;
      Check(it);
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
        Check(it);
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
          CheckParam(false, "Invalid binary pattern format");
          break;
        }
      }
      return PatternEntry(mask, value);
    }

    inline static PatternEntry ParseSymbol(PatternIterator& it)
    {
      Check(++it);
      const uint8_t val = static_cast<uint8_t>(*it);
      return PatternEntry(0xff, val);
    }

    inline static PatternEntry ParseRange(const PatternEntry& /*prev*/, PatternIterator& /*it*/)
    {
      throw std::domain_error("Binary pattern doesn't support ranges");
    }
  private:
    inline static uint8_t NibbleToMask(char c)
    {
      CheckParam(std::isxdigit(c) || ANY_NIBBLE_TEXT == c, "Invalid binary nibble format");
      return ANY_NIBBLE_TEXT == c
        ? 0 : 0xf;
    }

    inline static uint8_t NibbleToValue(char c)
    {
      CheckParam(std::isxdigit(c) || ANY_NIBBLE_TEXT == c, "Invalid binary nibble format");
      return ANY_NIBBLE_TEXT == c
        ? 0 : (std::isdigit(c) ? c - '0' : std::toupper(c) - 'A' + 10);
    }
  };

  struct RangedTraits
  {
    typedef std::pair<uint8_t, uint8_t> PatternEntry;
    BOOST_STATIC_ASSERT(sizeof(PatternEntry) == 2);
    typedef std::vector<PatternEntry> Pattern;

    inline static PatternEntry GetAnyByte()
    {
      return PatternEntry(0, 255);
    }

    inline static bool Match(const PatternEntry& lh, uint8_t rh)
    {
      return lh.first <= rh && rh <= lh.second;
    }

    inline static PatternEntry ParseNibbles(PatternIterator& it)
    {
      Check(it);
      const char hiNibble(*it);
      ++it;
      Check(it);
      const char loNibble(*it);
      const uint8_t val = NibbleToValue(hiNibble) * 16 + NibbleToValue(loNibble);
      return PatternEntry(val, val);
    }

    inline static PatternEntry ParseBinary(PatternIterator& /*it*/)
    {
      throw std::domain_error("Range pattern doesn't support binaries");
    }

    inline static PatternEntry ParseSymbol(PatternIterator& it)
    {
      Check(++it);
      const uint8_t val = static_cast<uint8_t>(*it);
      return PatternEntry(val, val);
    }

    inline static PatternEntry ParseRange(const PatternEntry& prev, PatternIterator& it)
    {
      ++it;
      CheckParam(prev.first == prev.second, "Invalid range format");
      const PatternEntry next = ParseNibbles(it);
      CheckParam(prev.first < next.second, "Invalid range format");
      return PatternEntry(prev.first, next.second);
    }
  private:
    inline static uint8_t NibbleToValue(char c)
    {
      CheckParam(0 != std::isxdigit(c), "Invalid range nibble value");
      return (std::isdigit(c) ? c - '0' : std::toupper(c) - 'A' + 10);
    }
  };

  struct BitmapTraits
  {
    typedef std::bitset<256> PatternEntry;
    typedef std::vector<PatternEntry> Pattern;

    inline static PatternEntry GetAnyByte()
    {
      return ~PatternEntry();
    }

    inline static bool Match(const PatternEntry& lh, uint8_t rh)
    {
      return lh.test(rh);
    }

    inline static PatternEntry ParseNibbles(PatternIterator& it)
    {
      const BinaryTraits::PatternEntry binPat = BinaryTraits::ParseNibbles(it);
      return Other2Bit<BinaryTraits>(binPat);
    }

    inline static PatternEntry ParseBinary(PatternIterator& it)
    {
      const BinaryTraits::PatternEntry binPat = BinaryTraits::ParseBinary(it);
      return Other2Bit<BinaryTraits>(binPat);
    }

    inline static PatternEntry ParseSymbol(PatternIterator& it)
    {
      Check(++it);
      const uint8_t val = static_cast<uint8_t>(*it);
      PatternEntry res;
      res[val] = true;
      return res;
    }

    inline static PatternEntry ParseRange(const PatternEntry& prev, PatternIterator& it)
    {
      ++it;
      CheckParam(prev.count() == 1, "Invalid range format");
      const RangedTraits::PatternEntry rngPat = RangedTraits::ParseNibbles(it);
      const PatternEntry next = Other2Bit<RangedTraits>(rngPat) << 1;
      PatternEntry res;
      for (PatternEntry cur = prev; cur != next; cur <<= 1)
      {
        res |= cur;
      }
      return res;
    }
  private:
    template<class Traits>
    inline static PatternEntry Other2Bit(const typename Traits::PatternEntry& pat)
    {
      PatternEntry val;
      for (uint_t idx = 0; idx < 256; ++idx)
      {
        val[idx] = Traits::Match(pat, static_cast<uint8_t>(idx));
      }
      return val;
    }
  };

  template<class Traits>
  void CompilePattern(const std::string& textPattern, typename Traits::Pattern& compiled)
  {
    std::stack<std::size_t> groupBegins;
    std::stack<std::pair<std::size_t, std::size_t> > groups;
    typename Traits::Pattern result;
    for (PatternIterator it(textPattern.begin(), textPattern.end()); it; ++it)
    {
      switch (*it)
      {
      case ANY_BYTE_TEXT:
        result.push_back(Traits::GetAnyByte());
        break;
      case SKIP_BYTES_TEXT:
        {
          const std::size_t skip = ParseSkipBytes(it);
          std::fill_n(std::back_inserter(result), skip, Traits::GetAnyByte());
        }
        break;
      case BINARY_MASK_TEXT:
        {
          const typename Traits::PatternEntry& entry = Traits::ParseBinary(it);
          result.push_back(entry);
        }
        break;
      case SYMBOL_TEXT:
        {
          const typename Traits::PatternEntry& entry = Traits::ParseSymbol(it);
          result.push_back(entry);
        }
        break;
      case RANGE_TEXT:
        {
          CheckParam(!result.empty(), "Invalid range format");
          typename Traits::PatternEntry& last = result.back();
          last = Traits::ParseRange(last, it);
        }
        break;
      case QUANTOR_BEGIN:
        {
          CheckParam(!result.empty(), "Invalid quantor format");
          if (const std::size_t mult = ParseQuantor(it))
          {
            typename Traits::Pattern dup;
            if (!groups.empty() && groups.top().second == result.size())
            {
              dup.assign(result.begin() + groups.top().first, result.end());
              groups.pop();
            }
            else
            {
              dup.push_back(result.back());
            }
            for (std::size_t idx = 0; idx < mult - 1; ++idx)
            {
              std::copy(dup.begin(), dup.end(), std::back_inserter(result));
            }
          }
        }
        break;
      case GROUP_BEGIN:
        groupBegins.push(result.size());
        break;
      case GROUP_END:
        {
          CheckParam(!groupBegins.empty(), "Group end without beginning");
          const std::pair<std::size_t, std::size_t> newGroup = std::make_pair(groupBegins.top(), result.size());
          groupBegins.pop();
          groups.push(newGroup);
        }
      case ' ':
      case '\n':
      case '\r':
      case '\t':
        break;
      default:
        {
          const typename Traits::PatternEntry& entry = Traits::ParseNibbles(it);
          result.push_back(entry);
        }
      }
    }
    compiled.swap(result);
  }

  template<class Traits>
  bool MatchByte(uint8_t lh, const typename Traits::PatternEntry& rh)
  {
    return Traits::Match(rh, lh);
  }

  template<class Traits>
  class Format : public DataFormat
  {
  public:
    Format(typename Traits::Pattern::const_iterator from, typename Traits::Pattern::const_iterator to, std::size_t offset)
      : Pat(from, to)
      , Offset(offset)
    {
    }

    virtual bool Match(const void* data, std::size_t size) const
    {
      if (Offset + Pat.size() > size)
      {
        return false;
      }
      const uint8_t* const typedData = static_cast<const uint8_t*>(data) + Offset;
      return std::equal(Pat.begin(), Pat.end(), typedData, &Traits::Match);
    }

    virtual std::size_t Search(const void* data, std::size_t size) const
    {
      if (Offset + Pat.size() > size)
      {
        return size;
      }
      const uint8_t* const typedData = static_cast<const uint8_t*>(data);
      const uint8_t* const typedEnd = typedData + size;
      const uint8_t* const result = std::search(typedData + Offset, typedEnd, Pat.begin(), Pat.end(), &MatchByte<Traits>);
      if (result == typedEnd)
      {
        return size;
      }
      return result - typedData - Offset;
    }
  private:
    const typename Traits::Pattern Pat;
    const std::size_t Offset;
  };

  class AlwaysMatchFormat : public DataFormat
  {
  public:
    explicit AlwaysMatchFormat(std::size_t offset)
      : Offset(offset)
    {
    }

    virtual bool Match(const void* /*data*/, std::size_t size) const
    {
      return Offset < size;
    }

    virtual std::size_t Search(const void* /*data*/, std::size_t size) const
    {
      return Offset < size
        ? 0
        : size;
    }
  private:
    const std::size_t Offset;
  };

  template<class Traits>
  static DataFormat::Ptr CreateDataFormat(const std::string& pattern)
  {
    typename Traits::Pattern pat;
    CompilePattern<Traits>(pattern, pat);
    const typename Traits::Pattern::const_iterator first = pat.begin();
    const typename Traits::Pattern::const_iterator last = pat.end();
    const typename Traits::Pattern::const_iterator firstNotAny = std::find_if(first, last, 
      std::bind2nd(std::not_equal_to<typename Traits::PatternEntry>(), Traits::GetAnyByte()));
    const std::size_t offset = std::distance(first, firstNotAny);
    return firstNotAny != last
      ? DataFormat::Ptr(new Format<Traits>(firstNotAny, last, offset))
      : DataFormat::Ptr(new AlwaysMatchFormat(offset));
  }
}

DataFormat::Ptr DataFormat::Create(const std::string& pattern)
{
  const bool hasBinaryMasks = pattern.find(BINARY_MASK_TEXT) != std::string::npos ||
                              pattern.find(ANY_NIBBLE_TEXT) != std::string::npos;
  const bool hasRanges = pattern.find(RANGE_TEXT) != std::string::npos;
  if (!hasRanges)
  {
    return CreateDataFormat<BinaryTraits>(pattern);
  }
  else if (hasRanges && !hasBinaryMasks)
  {
    return CreateDataFormat<RangedTraits>(pattern);
  }
  return CreateDataFormat<BitmapTraits>(pattern);
}
