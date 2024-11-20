/**
 *
 * @file
 *
 * @brief Analysis result implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "analysis/result.h"

#include "make_ptr.h"

#include <utility>

namespace Analysis
{
  class CalculatedResult : public Result
  {
  public:
    explicit CalculatedResult(std::size_t matchedSize, std::size_t unmatchedSize)
      : MatchedSize(matchedSize)
      , UnmatchedSize(unmatchedSize)
    {}

    std::size_t GetMatchedDataSize() const override
    {
      return MatchedSize;
    }

    std::size_t GetLookaheadOffset() const override
    {
      return UnmatchedSize;
    }

  private:
    const std::size_t MatchedSize;
    const std::size_t UnmatchedSize;
  };

  class UnmatchedResult : public Result
  {
  public:
    UnmatchedResult(Binary::Format::Ptr format, Binary::Container::Ptr data)
      : Format(std::move(format))
      , RawData(std::move(data))
    {}

    std::size_t GetMatchedDataSize() const override
    {
      return 0;
    }

    std::size_t GetLookaheadOffset() const override
    {
      return Format->NextMatchOffset(*RawData);
    }

  private:
    const Binary::Format::Ptr Format;
    const Binary::Container::Ptr RawData;
  };
}  // namespace Analysis

namespace Analysis
{
  Result::Ptr CreateMatchedResult(std::size_t matchedSize)
  {
    return MakePtr<CalculatedResult>(matchedSize, 0);
  }

  Result::Ptr CreateUnmatchedResult(Binary::Format::Ptr format, Binary::Container::Ptr data)
  {
    return MakePtr<UnmatchedResult>(std::move(format), std::move(data));
  }

  Result::Ptr CreateUnmatchedResult(std::size_t unmatchedSize)
  {
    return MakePtr<CalculatedResult>(0, unmatchedSize);
  }
}  // namespace Analysis
