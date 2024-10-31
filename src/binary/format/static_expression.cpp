/**
 *
 * @file
 *
 * @brief  Static predicates helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/format/static_expression.h"

#include <algorithm>

namespace Binary::FormatDSL
{
  // result[x] = backward offset of suffix size x
  std::vector<std::size_t> StaticPattern::GetSuffixOffsets() const
  {
    // assume longer suffixes requires longer offsets
    const auto size = GetSize();
    std::vector<std::size_t> result(size + 1);
    for (std::size_t offset = size; offset > 0; --offset)
    {
      if (const auto maxSize = FindMaxSuffixMatchSize(offset))
      {
        std::fill_n(result.begin() + 1, maxSize, offset);
      }
    }
    return result;
  }

  std::size_t StaticPattern::FindPrefix(std::size_t prefixSize) const
  {
    const StaticPredicate* const prefixBegin = Begin();
    const StaticPredicate* const prefixEnd = Begin() + prefixSize;
    const std::size_t patternSize = GetSize();
    const std::size_t startOffset = 1;
    const StaticPredicate* start = prefixBegin + startOffset;
    for (std::size_t offset = startOffset;; ++offset, ++start)
    {
      const std::size_t windowSize = prefixSize + offset;
      if (patternSize >= windowSize)
      {
        // pattern: ssssss...
        // prefix:   sssssx
        // offset=1
        if (std::equal(prefixBegin, prefixEnd, start, &StaticPredicate::AreIntersected))
        {
          return offset;
        }
      }
      else
      {
        if (patternSize == offset)
        {
          // all prefix is out of patter
          return offset;
        }
        // pattern: ....
        // prefix:    sssx
        // out of pattern=2
        const std::size_t outOfPattern = windowSize - patternSize;
        if (std::equal(prefixBegin, prefixEnd - outOfPattern, start, &StaticPredicate::AreIntersected))
        {
          return offset;
        }
      }
    }
  }

  std::size_t StaticPattern::FindMaxSuffixMatchSize(std::size_t offset) const
  {
    const auto* const begin = Begin();
    std::size_t size = 0;
    const auto* suffix = End() - 1;
    const auto* pattern = suffix - offset;
    while (pattern >= begin && StaticPredicate::AreIntersected(*suffix, *pattern))
    {
      --pattern;
      --suffix;
      ++size;
    }
    return pattern < begin ? GetSize() : size;
  }
}  // namespace Binary::FormatDSL
