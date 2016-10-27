/**
*
* @file
*
* @brief  Static predicates helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "static_expression.h"

namespace Binary
{
namespace FormatDSL
{
  std::size_t StaticPattern::FindSuffix(std::size_t suffixSize) const
  {
    const StaticPredicate* const suffixBegin = End() - suffixSize;
    const StaticPredicate* const suffixEnd = End();
    const std::size_t patternSize = GetSize();
    const std::size_t startOffset = 1;
    const StaticPredicate* start = suffixBegin - startOffset;
    for (std::size_t offset = startOffset; ; ++offset, --start)
    {
      const std::size_t windowSize = suffixSize + offset;
      if (patternSize >= windowSize)
      {
        //pattern:  ......sssssss
        //suffix:        xssssss
        //offset=1
        if (std::equal(suffixBegin, suffixEnd, start, &StaticPredicate::AreIntersected))
        {
          return offset;
        }
      }
      else
      {
        if (patternSize == offset)
        {
          //all suffix is out of pattern
          return offset;
        }
        //pattern:   .......
        //suffix:  xssssss
        //out of pattern=2
        const std::size_t outOfPattern = windowSize - patternSize;
        if (std::equal(suffixBegin + outOfPattern, suffixEnd, Begin(), &StaticPredicate::AreIntersected))
        {
          return offset;
        }
      }
    }
  }

  std::size_t StaticPattern::FindPrefix(std::size_t prefixSize) const
  {
    const StaticPredicate* const prefixBegin = Begin();
    const StaticPredicate* const prefixEnd = Begin() + prefixSize;
    const std::size_t patternSize = GetSize();
    const std::size_t startOffset = 1;
    const StaticPredicate* start = prefixBegin + startOffset;
    for (std::size_t offset = startOffset; ; ++offset, ++start)
    {
      const std::size_t windowSize = prefixSize + offset;
      if (patternSize >= windowSize)
      {
        //pattern: ssssss...
        //prefix:   sssssx
        //offset=1
        if (std::equal(prefixBegin, prefixEnd, start, &StaticPredicate::AreIntersected))
        {
          return offset;
        }
      }
      else
      {
        if (patternSize == offset)
        {
          //all prefix is out of patter
          return offset;
        }
        //pattern: ....
        //prefix:    sssx
        //out of pattern=2
        const std::size_t outOfPattern = windowSize - patternSize;
        if (std::equal(prefixBegin, prefixEnd - outOfPattern, start, &StaticPredicate::AreIntersected))
        {
          return offset;
        }
      }
    }
  }
}
}
