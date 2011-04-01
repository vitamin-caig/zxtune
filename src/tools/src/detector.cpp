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

  const char ANY_BIT_TEXT = 'x';
  const char ZERO_BIT_TEXT = '0';
  const char ONE_BIT_TEXT = '1';

  struct PatternEntry
  {
    uint8_t Mask;
    uint8_t Value;

    PatternEntry()
      : Mask(), Value()
    {
    }

    PatternEntry(uint8_t mask, uint8_t val)
      : Mask(mask), Value(val)
    {
    }
  };

  BOOST_STATIC_ASSERT(sizeof(PatternEntry) == 2);

  bool operator == (const PatternEntry& lh, uint8_t rh)
  {
    return lh.Mask ? ((lh.Mask & rh) == lh.Value) : true;
  }

  bool operator == (uint8_t lh, const PatternEntry& rh)
  {
    return rh == lh;
  }

  typedef std::vector<PatternEntry> BinaryPattern;

  inline uint8_t NibbleToMask(char c)
  {
    assert(std::isxdigit(c) || ANY_NIBBLE_TEXT == c);
    return ANY_NIBBLE_TEXT == c
      ? 0 : 0xf;
  }

  inline uint8_t NibbleToValue(char c)
  {
    assert(std::isxdigit(c) || ANY_NIBBLE_TEXT == c);
    return ANY_NIBBLE_TEXT == c
      ? 0 : (std::isdigit(c) ? c - '0' : std::toupper(c) - 'A' + 10);
  }

  typedef RangeIterator<std::string::const_iterator> PatternIterator;

  std::size_t ParseSkipBytes(PatternIterator& it)
  {
    std::size_t skip = 0;
    while (SKIP_BYTES_TEXT != *++it)
    {
      assert((it && std::isdigit(*it)) || !"Invalid pattern format");
      skip = skip * 10 + (*it - '0');
    }
    return skip;
  }

  PatternEntry ParseNibbles(PatternIterator& it)
  {
    const char hiNibble(*it);
    ++it;
    assert(it || !"Invalid pattern format");
    const char loNibble(*it);
    const uint8_t mask = NibbleToMask(hiNibble) * 16 + NibbleToMask(loNibble);
    const uint8_t value = NibbleToValue(hiNibble) * 16 + NibbleToValue(loNibble);
    return PatternEntry(mask, value);
  }

  PatternEntry ParseBinary(PatternIterator& it)
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

  std::size_t DetectFormatLookahead(const uint8_t* data, std::size_t size, const BinaryPattern& pattern)
  {
    if (pattern.size() > size)
    {
      return size;
    }
    return std::search(data, data + size, pattern.begin(), pattern.end()) - data;
  }

  bool DetectFormat(const uint8_t* data, std::size_t size, const BinaryPattern& pattern)
  {
    if (pattern.size() > size)
    {
      return false;
    }
    return std::equal(pattern.begin(), pattern.end(), data);
  }

  void CompileDetectPattern(const std::string& textPattern, BinaryPattern& binPattern)
  {
    static const PatternEntry ANY_BYTE(0, 0);

    BinaryPattern result;
    for (PatternIterator it(textPattern.begin(), textPattern.end()); it; ++it)
    {
      switch (*it)
      {
      case ANY_BYTE_TEXT:
        result.push_back(ANY_BYTE);
        break;
      case SKIP_BYTES_TEXT:
        {
          const std::size_t skip = ParseSkipBytes(it);
          std::fill_n(std::back_inserter(result), skip, ANY_BYTE);
        }
        break;
      case BINARY_MASK_TEXT:
        {
          const PatternEntry& entry = ParseBinary(it);
          result.push_back(entry);
        }
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

  class DetectionResultImpl : public DetectionResult
  {
  public:
    DetectionResultImpl(std::size_t matchedSize, std::size_t lookAhead)
      : MatchedSize(matchedSize)
      , LookAhead(lookAhead)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      return MatchedSize;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      return LookAhead;
    }
  private:
    const std::size_t MatchedSize;
    const std::size_t LookAhead;
  };

  class DataFormatImpl : public DataFormat
  {
  public:
    explicit DataFormatImpl(const std::string& pattern)
    {
      CompileDetectPattern(pattern, Pattern);
    }

    virtual bool Match(const void* data, std::size_t size) const
    {
      return DetectFormat(static_cast<const uint8_t*>(data), size, Pattern);
    }

    virtual std::size_t Search(const void* data, std::size_t size) const
    {
      return DetectFormatLookahead(static_cast<const uint8_t*>(data), size, Pattern);
    }
  private:
    BinaryPattern Pattern;
  };
}

DetectionResult::Ptr DetectionResult::Create(std::size_t matchedSize, std::size_t lookAhead)
{
  return boost::make_shared<DetectionResultImpl>(matchedSize, lookAhead);
}

DataFormat::Ptr DataFormat::Create(const std::string& pattern)
{
  return boost::make_shared<DataFormatImpl>(pattern);
}
