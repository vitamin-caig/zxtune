/**
* 
* @file
*
* @brief Analysis result implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <analysis/result.h>

namespace
{
  class CalculatedResult : public Analysis::Result
  {
  public:
    explicit CalculatedResult(std::size_t matchedSize, std::size_t unmatchedSize)
      : MatchedSize(matchedSize)
      , UnmatchedSize(unmatchedSize)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      return MatchedSize;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      return UnmatchedSize;
    }
  private:
    const std::size_t MatchedSize;
    const std::size_t UnmatchedSize;
  };

  class UnmatchedResult : public Analysis::Result
  {
  public:
    UnmatchedResult(Binary::Format::Ptr format, Binary::Container::Ptr data)
      : Format(format)
      , RawData(data)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      return 0;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      return Format->NextMatchOffset(*RawData);
    }
  private:
    const Binary::Format::Ptr Format;
    const Binary::Container::Ptr RawData;
  };
}

namespace Analysis
{
  Result::Ptr CreateMatchedResult(std::size_t matchedSize)
  {
    return MakePtr<CalculatedResult>(matchedSize, 0);
  }

  Result::Ptr CreateUnmatchedResult(Binary::Format::Ptr format, Binary::Container::Ptr data)
  {
    return MakePtr<UnmatchedResult>(format, data);
  }

  Result::Ptr CreateUnmatchedResult(std::size_t unmatchedSize)
  {
    return MakePtr<CalculatedResult>(0, unmatchedSize);
  }
}
