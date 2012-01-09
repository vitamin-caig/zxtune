/*
Abstract:
  Format detector implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <iterator.h>
#include <types.h>
//library includes
#include <binary/format.h>
//std includes
#include <bitset>
#include <cassert>
#include <cctype>
#include <stack>
#include <stdexcept>
#include <vector>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace Binary;

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
    typedef std::pair<uint_t, uint_t> PatternEntry;
    typedef std::vector<PatternEntry> Pattern;

    inline static PatternEntry GetAnyByte()
    {
      return PatternEntry(0, 0);
    }

    inline static bool Match(const PatternEntry& lh, uint_t rh)
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
      const uint_t mask = NibbleToMask(hiNibble) * 16 + NibbleToMask(loNibble);
      const uint_t value = NibbleToValue(hiNibble) * 16 + NibbleToValue(loNibble);
      return PatternEntry(mask, value);
    }

    inline static PatternEntry ParseBinary(PatternIterator& it)
    {
      uint_t mask = 0;
      uint_t value = 0;
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
    inline static uint_t NibbleToMask(char c)
    {
      CheckParam(std::isxdigit(c) || ANY_NIBBLE_TEXT == c, "Invalid binary nibble format");
      return ANY_NIBBLE_TEXT == c
        ? 0 : 0xf;
    }

    inline static uint_t NibbleToValue(char c)
    {
      CheckParam(std::isxdigit(c) || ANY_NIBBLE_TEXT == c, "Invalid binary nibble format");
      return ANY_NIBBLE_TEXT == c
        ? 0 : (std::isdigit(c) ? c - '0' : std::toupper(c) - 'A' + 10);
    }
  };

  struct RangedTraits
  {
    typedef std::pair<uint_t, uint_t> PatternEntry;
    typedef std::vector<PatternEntry> Pattern;

    inline static PatternEntry GetAnyByte()
    {
      return PatternEntry(0, 255);
    }

    inline static bool Match(const PatternEntry& lh, uint_t rh)
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
      const uint_t val = NibbleToValue(hiNibble) * 16 + NibbleToValue(loNibble);
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
    inline static uint_t NibbleToValue(char c)
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

    inline static bool Match(const PatternEntry& lh, uint_t rh)
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
        val[idx] = Traits::Match(pat, idx);
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

  class FastSearchFormat : public Format
  {
  public:
    typedef boost::array<std::size_t, 256> PatternRow;
    typedef std::vector<PatternRow> PatternMatrix;

    FastSearchFormat(const PatternMatrix& mtx, std::size_t offset)
      : Offset(offset)
      , Pat(mtx)
      , PatBegin(&Pat[0])
      , PatEnd(PatBegin + Pat.size())
    {
    }

    virtual bool Match(const void* data, std::size_t size) const
    {
      if (Offset + Pat.size() > size)
      {
        return false;
      }
      const uint8_t* typedData = static_cast<const uint8_t*>(data) + Offset;
      return 0 == Search(typedData);
    }

    virtual std::size_t Search(const void* data, std::size_t size) const
    {
      if (Offset + Pat.size() > size)
      {
        return size;
      }
      const uint8_t* const typedData = static_cast<const uint8_t*>(data);
      for (std::size_t cursor = Offset; cursor <= size - Pat.size(); )
      {
        if (const std::size_t offset = Search(typedData + cursor))
        {
          cursor += offset;
        }
        else
        {
          return cursor - Offset;
        }
      }
      return size;
    }

    template<class Traits>
    static Ptr Create(typename Traits::Pattern::const_iterator from, typename Traits::Pattern::const_iterator to, std::size_t offset)
    {
      //Pass1
      //matrix[pos][char] = Pattern.At(pos).IsMatchedFor(char)
      PatternMatrix tmp;
      for (typename Traits::Pattern::const_iterator it = from; it != to; ++it)
      {
        PatternRow row;
        for (uint_t idx = 0; idx != 256; ++idx)
        {
          row[idx] = Traits::Match(*it, idx);
        }
        tmp.push_back(row);
      }
      //Pass2
      //matrix[pos][char] = delta, Pattern.At(pos - delta).IsMatchedFor(char)
      for (uint_t idx = 0; idx != 256; ++idx)
      {
        std::size_t offset = 1;//default offset
        for (PatternMatrix::iterator it = tmp.begin(), lim = tmp.end(); it != lim; ++it, ++offset)
        {
          PatternRow& row = *it;
          if (row[idx])
          {
            offset = 0;
          }
          row[idx] = offset;
        }
      }
      return boost::make_shared<FastSearchFormat>(tmp, offset);
    }
  private:
    std::size_t Search(const uint8_t* data) const
    {
      const uint8_t* dataEnd = data + Pat.size();
      for (const PatternRow* it = PatEnd; it != PatBegin; )
      {
        const PatternRow& row = *--it;
        if (std::size_t offset = row[*--dataEnd])
        {
          return offset;
        }
      }
      return 0;
    }
  private:
    const std::size_t Offset;
    const PatternMatrix Pat;
    const PatternRow* const PatBegin;
    const PatternRow* const PatEnd;
  };

  class AlwaysMatchFormat : public Format
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

    static Ptr Create(std::size_t offset)
    {
      return boost::make_shared<AlwaysMatchFormat>(offset);
    }

  private:
    const std::size_t Offset;
  };

  template<class Traits>
  static Format::Ptr CreateFormat(const std::string& pattern)
  {
    typename Traits::Pattern pat;
    CompilePattern<Traits>(pattern, pat);
    const typename Traits::Pattern::const_iterator first = pat.begin();
    const typename Traits::Pattern::const_iterator last = pat.end();
    const typename Traits::Pattern::const_iterator firstNotAny = std::find_if(first, last, 
      std::bind2nd(std::not_equal_to<typename Traits::PatternEntry>(), Traits::GetAnyByte()));
    const std::size_t offset = std::distance(first, firstNotAny);
    return firstNotAny != last
      ? FastSearchFormat::Create<Traits>(firstNotAny, last, offset)
      : AlwaysMatchFormat::Create(offset);
  }
}

namespace Binary
{
  Format::Ptr Format::Create(const std::string& pattern)
  {
    const bool hasBinaryMasks = pattern.find(BINARY_MASK_TEXT) != std::string::npos ||
                                pattern.find(ANY_NIBBLE_TEXT) != std::string::npos;
    const bool hasRanges = pattern.find(RANGE_TEXT) != std::string::npos;
    if (!hasRanges)
    {
      return CreateFormat<BinaryTraits>(pattern);
    }
    else if (hasRanges && !hasBinaryMasks)
    {
      return CreateFormat<RangedTraits>(pattern);
    }
    return CreateFormat<BitmapTraits>(pattern);
  }
}
