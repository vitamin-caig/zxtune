/**
*
* @file
*
* @brief  Matching-only format implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "static_expression.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>

namespace Binary
{
  class MatchOnlyFormatBase : public Format
  {
  public:
    virtual std::size_t NextMatchOffset(const Data& data) const
    {
      return data.Size();
    }
  };

  class FuzzyMatchOnlyFormat : public MatchOnlyFormatBase
  {
  public:
    FuzzyMatchOnlyFormat(const FormatDSL::StaticPattern& mtx, std::size_t offset, std::size_t minSize)
      : Offset(offset)
      , MinSize(std::max(minSize, mtx.GetSize() + offset))
      , Pattern(mtx)
    {
    }

    virtual bool Match(const Data& data) const
    {
      if (data.Size() < MinSize)
      {
        return false;
      }
      const uint8_t* const typedData = static_cast<const uint8_t*>(data.Start()) + Offset;
      for (std::size_t idx = 0, lim = Pattern.GetSize(); idx != lim; ++idx)
      {
        if (!Pattern.Get(idx).Match(typedData[idx]))
        {
          return false;
        }
      }
      return true;
    }

    static Ptr Create(const FormatDSL::StaticPattern& expr, std::size_t startOffset, std::size_t minSize)
    {
      return MakePtr<FuzzyMatchOnlyFormat>(expr, startOffset, minSize);
    }
  private:
    const std::size_t Offset;
    const std::size_t MinSize;
    const FormatDSL::StaticPattern Pattern;
  };

  class ExactMatchOnlyFormat : public MatchOnlyFormatBase
  {
  public:
    typedef std::vector<uint8_t> PatternMatrix;

    ExactMatchOnlyFormat(const PatternMatrix& mtx, std::size_t offset, std::size_t minSize)
      : Offset(offset)
      , MinSize(std::max(minSize, mtx.size() + offset))
      , Pattern(mtx)
    {
    }

    virtual bool Match(const Data& data) const
    {
      if (data.Size() < MinSize)
      {
        return false;
      }
      const uint8_t* const patternStart = &Pattern.front();
      const uint8_t* const patternEnd = patternStart + Pattern.size();
      const uint8_t* const typedDataStart = static_cast<const uint8_t*>(data.Start()) + Offset;
      return std::equal(patternStart, patternEnd, typedDataStart);
    }

    static Ptr TryCreate(const FormatDSL::StaticPattern& pattern, std::size_t startOffset, std::size_t minSize)
    {
      const std::size_t patternSize = pattern.GetSize();
      PatternMatrix tmp(patternSize);
      for (std::size_t idx = 0; idx != patternSize; ++idx)
      {
        const FormatDSL::StaticToken& tok = pattern.Get(idx);
        if (const uint_t* single = tok.GetSingle())
        {
          tmp[idx] = *single;
        }
        else
        {
          return Ptr();
        }
      }
      return MakePtr<ExactMatchOnlyFormat>(tmp, startOffset, minSize);
    }
  private:
    const std::size_t Offset;
    const std::size_t MinSize;
    const PatternMatrix Pattern;
  };

  Format::Ptr CreateMatchingFormatFromTokens(const FormatDSL::Expression& expr, std::size_t minSize)
  {
    const FormatDSL::StaticPattern pattern(expr.Tokens());
    const std::size_t startOffset = expr.StartOffset();
    if (const Format::Ptr exact = ExactMatchOnlyFormat::TryCreate(pattern, startOffset, minSize))
    {
      return exact;
    }
    else
    {
      return FuzzyMatchOnlyFormat::Create(pattern, startOffset, minSize);
    }
  }
}

namespace Binary
{
  Format::Ptr CreateMatchOnlyFormat(const std::string& pattern)
  {
    return CreateMatchOnlyFormat(pattern, 0);
  }

  Format::Ptr CreateMatchOnlyFormat(const std::string& pattern, std::size_t minSize)
  {
    const FormatDSL::Expression::Ptr expr = FormatDSL::Expression::Parse(pattern);
    return CreateMatchingFormatFromTokens(*expr, minSize);
  }
}
