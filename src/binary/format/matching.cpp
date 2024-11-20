/**
 *
 * @file
 *
 * @brief  Matching-only format implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/format/static_expression.h"

#include "binary/format_factories.h"

#include "make_ptr.h"
#include "string_view.h"

namespace Binary
{
  class MatchOnlyFormatBase : public Format
  {
  public:
    std::size_t NextMatchOffset(View data) const override
    {
      return data.Size();
    }
  };

  class FuzzyMatchOnlyFormat : public MatchOnlyFormatBase
  {
  public:
    FuzzyMatchOnlyFormat(FormatDSL::StaticPattern mtx, std::size_t offset, std::size_t minSize)
      : Offset(offset)
      , MinSize(std::max(minSize, mtx.GetSize() + offset))
      , Pattern(std::move(mtx))
    {}

    bool Match(View data) const override
    {
      return data.Size() >= MinSize && Pattern.Match(data.SubView(Offset).As<uint8_t>());
    }

    static Ptr Create(FormatDSL::StaticPattern expr, std::size_t startOffset, std::size_t minSize)
    {
      return MakePtr<FuzzyMatchOnlyFormat>(std::move(expr), startOffset, minSize);
    }

  private:
    const std::size_t Offset;
    const std::size_t MinSize;
    const FormatDSL::StaticPattern Pattern;
  };

  class ExactMatchOnlyFormat : public MatchOnlyFormatBase
  {
  public:
    using PatternMatrix = std::vector<uint8_t>;

    ExactMatchOnlyFormat(PatternMatrix mtx, std::size_t offset, std::size_t minSize)
      : Offset(offset)
      , MinSize(std::max(minSize, mtx.size() + offset))
      , Pattern(std::move(mtx))
    {}

    bool Match(View data) const override
    {
      if (data.Size() < MinSize)
      {
        return false;
      }
      const uint8_t* const patternStart = Pattern.data();
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
        const FormatDSL::StaticPredicate& pred = pattern.Get(idx);
        if (const uint_t* single = pred.GetSingle())
        {
          tmp[idx] = *single;
        }
        else
        {
          return {};
        }
      }
      return MakePtr<ExactMatchOnlyFormat>(std::move(tmp), startOffset, minSize);
    }

  private:
    const std::size_t Offset;
    const std::size_t MinSize;
    const PatternMatrix Pattern;
  };

  Format::Ptr CreateMatchingFormatFromPredicates(const FormatDSL::Expression& expr, std::size_t minSize)
  {
    FormatDSL::StaticPattern pattern(expr.Predicates());
    const std::size_t startOffset = expr.StartOffset();
    if (Format::Ptr exact = ExactMatchOnlyFormat::TryCreate(pattern, startOffset, minSize))
    {
      return exact;
    }
    else
    {
      return FuzzyMatchOnlyFormat::Create(std::move(pattern), startOffset, minSize);
    }
  }
}  // namespace Binary

namespace Binary
{
  Format::Ptr CreateMatchOnlyFormat(StringView pattern)
  {
    return CreateMatchOnlyFormat(pattern, 0);
  }

  Format::Ptr CreateMatchOnlyFormat(StringView pattern, std::size_t minSize)
  {
    const auto expr = FormatDSL::Expression::Parse(pattern);
    return CreateMatchingFormatFromPredicates(*expr, minSize);
  }
}  // namespace Binary
