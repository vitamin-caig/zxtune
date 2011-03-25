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
//std includes
#include <cassert>
#include <cctype>

namespace
{
  const char ANY_BYTE_TEXT = '?';
  const char SKIP_BYTES_TEXT = '+';

  //hybyte=value lobyte=mask
  const uint16_t ANY_BYTE_BIN = 0x0000;

  uint16_t MakeByteBin(uint_t byte)
  {
    return byte * 256 + 0xff;
  }

  bool MatchPatternBinByByte(uint16_t bin, uint8_t byte)
  {
    return bin == ANY_BYTE_BIN ||
      MakeByteBin(byte) == bin;
  }

  bool MatchByteByPatternBin(uint8_t byte, uint16_t bin)
  {
    return MatchPatternBinByByte(bin, byte);
  }

  inline uint8_t ToHex(char c)
  {
    assert(std::isxdigit(c));
    return std::isdigit(c) ? c - '0' : std::toupper(c) - 'A' + 10;
  }

  typedef RangeIterator<std::string::const_iterator> PatternIterator;

  std::size_t GetBytesToSkip(PatternIterator& it)
  {
    std::size_t skip = 0;
    while (SKIP_BYTES_TEXT != *++it)
    {
      assert((it && std::isdigit(*it)) || !"Invalid pattern format");
      skip = skip * 10 + (*it - '0');
    }
    return skip;
  }

  uint8_t GetByte(PatternIterator& it)
  {
    const char sym(*it);
    ++it;
    assert(it || !"Invalid pattern format");
    const char sym1(*it);
    return ToHex(sym) * 16 + ToHex(sym1);
  }
}

bool DetectFormat(const uint8_t* data, std::size_t size, const std::string& pattern)
{
  BinaryPattern binary;
  CompileDetectPattern(pattern, binary);
  return DetectFormat(data, size, binary);
}

bool DetectFormat(const uint8_t* data, std::size_t size, const BinaryPattern& pattern)
{
  if (pattern.size() > size)
  {
    return false;
  }
  return std::equal(pattern.begin(), pattern.end(), data, &MatchPatternBinByByte);
}

std::size_t DetectFormatLookahead(const uint8_t* data, std::size_t size, const BinaryPattern& pattern)
{
  if (pattern.size() > size)
  {
    return size;
  }
  return std::search(data, data + size, pattern.begin(), pattern.end(), &MatchByteByPatternBin) - data;
}

void CompileDetectPattern(const std::string& textPattern, BinaryPattern& binPattern)
{
  BinaryPattern result;
  for (PatternIterator it(textPattern.begin(), textPattern.end()); it; ++it)
  {
    const char sym(*it);
    if (ANY_BYTE_TEXT == sym)
    {
      result.push_back(ANY_BYTE_BIN);
    }
    else if (SKIP_BYTES_TEXT == sym)//skip
    {
      const std::size_t skip = GetBytesToSkip(it);
      std::fill_n(std::back_inserter(result), skip, ANY_BYTE_BIN);
    }
    else
    {
      const uint8_t byte = GetByte(it);
      result.push_back(MakeByteBin(byte));
    }
  }
  binPattern.swap(result);
}

