/*
Abstract:
  Format detector implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "expression.h"
//common includes
#include <contract.h>
#include <tools.h>
//library includes
#include <binary/format.h>
//std includes
#include <limits>
#include <vector>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace Binary;

  class StaticToken
  {
  public:
    StaticToken(Token::Ptr tok)
      : Mask()
      , Count()
    {
      for (uint_t idx = 0; idx != 256; ++idx)
      {
        if (tok->Match(idx))
        {
          Set(idx);
        }
      }
    }

    explicit StaticToken(uint_t val)
      : Mask()
      , Count()
    {
      Set(val);
    }

    bool Match(uint_t val) const
    {
      return Get(val);
    }

    bool IsAny() const
    {
      return Count == 256;
    }

    static bool AreIntersected(const StaticToken& lh, const StaticToken& rh)
    {
      if (lh.IsAny() || rh.IsAny())
      {
        return true;
      }
      else
      {
        for (uint_t idx = 0; idx != ElementsCount; ++idx)
        {
          if (0 != (lh.Mask[idx] & rh.Mask[idx]))
          {
            return true;
          }
        }
        return false;
      }
    }
  private:
    void Set(uint_t idx)
    {
      Require(!Get(idx));
      const uint_t bit = idx % BitsPerElement;
      const std::size_t offset = idx / BitsPerElement;
      const ElementType mask = ElementType(1) << bit;
      Mask[offset] |= mask;
      ++Count;
    }

    bool Get(uint_t idx) const
    {
      const uint_t bit = idx % BitsPerElement;
      const std::size_t offset = idx / BitsPerElement;
      const ElementType mask = ElementType(1) << bit;
      return 0 != (Mask[offset] & mask);
    }
  private:
    typedef uint_t ElementType;
    static const std::size_t BitsPerElement = 8 * sizeof(ElementType);
    static const std::size_t ElementsCount = 256 / BitsPerElement;
    boost::array<ElementType, ElementsCount> Mask;
    uint_t Count;
  };

  class StaticPattern
  {
  public:
    explicit StaticPattern(ObjectIterator<Token::Ptr>::Ptr iter)
    {
      for (; iter->IsValid(); iter->Next())
      {
        Data.push_back(StaticToken(iter->Get()));
      }
    }

    template<class T>
    StaticPattern(T from, T to)
      : Data(from, to)
    {
    }

    std::size_t GetSize() const
    {
      return Data.size();
    }

    const StaticToken& Get(std::size_t idx) const
    {
      return Data[idx];
    }

    StaticPattern GetSuffix(std::size_t size) const
    {
      Require(size <= GetSize());
      return StaticPattern(Data.end() - size, Data.end());
    }

    //return back offset
    std::size_t FindSuffix(const StaticPattern& suffix) const
    {
      const std::size_t patternSize = GetSize();
      const std::size_t suffixSize = suffix.GetSize();
      const std::size_t startOffset = 1;
      const StaticToken* start = End() - suffixSize - startOffset;
      for (std::size_t offset = startOffset; ; ++offset, --start)
      {
        const std::size_t windowSize = suffixSize + offset;
        if (patternSize >= windowSize)
        {
          //pattern:  ......sssssss
          //suffix:        xssssss
          //offset=1
          if (std::equal(suffix.Begin(), suffix.End(), start, &StaticToken::AreIntersected))
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
          if (std::equal(suffix.Begin() + outOfPattern, suffix.End(), Begin(), &StaticToken::AreIntersected))
          {
            return offset;
          }
        }
      }
    }
  private:
    const StaticToken* Begin() const
    {
      return &Data.front();
    }

    const StaticToken* End() const
    {
      return &Data.back() + 1;
    }
  private:
    std::vector<StaticToken> Data;
  };

  class FastSearchFormat : public Format
  {
  public:
    typedef boost::array<uint8_t, 256> PatternRow;
    typedef std::vector<PatternRow> PatternMatrix;

    FastSearchFormat(const PatternMatrix& mtx, std::size_t offset, std::size_t minSize)
      : Offset(offset)
      , MinSize(std::max(minSize, mtx.size() + offset))
      , Pat(mtx.rbegin(), mtx.rend())
      , PatRBegin(&Pat[0])
      , PatREnd(PatRBegin + Pat.size())
    {
    }

    virtual bool Match(const Data& data) const
    {
      if (data.Size() < MinSize)
      {
        return false;
      }
      const std::size_t endOfPat = Offset + Pat.size();
      const uint8_t* typedDataLast = static_cast<const uint8_t*>(data.Start()) + endOfPat - 1;
      return 0 == SearchBackward(typedDataLast);
    }

    virtual std::size_t Search(const Data& data) const
    {
      const std::size_t size = data.Size();
      if (size < MinSize)
      {
        return size;
      }
      const uint8_t* const typedData = static_cast<const uint8_t*>(data.Start());
      const std::size_t endOfPat = Offset + Pat.size();
      const uint8_t* const scanStart = typedData + endOfPat - 1;
      const uint8_t* const scanStop = typedData + size;
      for (const uint8_t* scanPos = scanStart; scanPos < scanStop; )
      {
        if (const std::size_t offset = SearchBackward(scanPos))
        {
          scanPos += offset;
        }
        else
        {
          return scanPos - scanStart;
        }
      }
      return size;
    }

    static Ptr Create(const Expression& expr, std::size_t minSize)
    {
      const StaticPattern pattern(expr.Tokens());
      const std::size_t patternSize = pattern.GetSize();
      PatternMatrix tmp(patternSize);
      for (uint_t sym = 0; sym != 256; ++sym)
      {
        for (std::size_t pos = 0, offset = 1; pos != patternSize; ++pos, ++offset)
        {
          const StaticToken& tok = pattern.Get(pos);
          if (tok.Match(sym))
          {
            offset = 0;
          }
          else
          {
            Require(in_range<std::size_t>(offset, 0, std::numeric_limits<PatternRow::value_type>::max()));
            tmp[pos][sym] = static_cast<PatternRow::value_type>(offset);
          }
        }
      }
      /*
       Increase offset using suffixes.
       Average offsets (standard big file test):
         without increasing: 2.17
         with increasing: 2.27
         with precise increasing: 2.28

       Due to high complexity of precise detection, simple increasing is used
      */
      for (std::size_t pos = 0; pos != patternSize - 1; ++pos)
      {
        PatternRow& row = tmp[pos];
        const std::size_t suffixLen = patternSize - pos - 1;
        const StaticPattern suffix = pattern.GetSuffix(suffixLen);
        const std::size_t offset = pattern.FindSuffix(suffix);
        const std::size_t availOffset = std::min<std::size_t>(offset, std::numeric_limits<PatternRow::value_type>::max());
        for (uint_t sym = 0; sym != 256; ++sym)
        {
          if (uint8_t& curOffset = row[sym])
          {
            curOffset = std::max(curOffset, static_cast<PatternRow::value_type>(availOffset));
          }
        }
      }
      //Each matrix element specifies forward movement of reversily matched pattern for specified symbol. =0 means symbol match
      return boost::make_shared<FastSearchFormat>(tmp, expr.StartOffset(), minSize);
    }
  private:
    std::size_t SearchBackward(const uint8_t* data) const
    {
      if (const std::size_t offset = (*PatRBegin)[*data])
      {
        return offset;
      }
      --data;
      for (const PatternRow* it = PatRBegin + 1; it != PatREnd; ++it, --data)
      {
        const PatternRow& row = *it;
        if (const std::size_t offset = row[*data])
        {
          return offset;
        }
      }
      return 0;
    }
  private:
    const std::size_t Offset;
    const std::size_t MinSize;
    const PatternMatrix Pat;
    const PatternRow* const PatRBegin;
    const PatternRow* const PatREnd;
  };
}

namespace Binary
{
  Format::Ptr Format::Create(const std::string& pattern, std::size_t minSize)
  {
    const Expression::Ptr expr = Expression::Parse(pattern);
    return FastSearchFormat::Create(*expr, minSize);
  }
}
