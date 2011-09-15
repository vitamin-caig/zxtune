/*
Abstract:
  Detection result implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "detection_result.h"
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  class DetectionResultImpl : public DetectionResult
  {
  public:
    explicit DetectionResultImpl(std::size_t matchedSize, std::size_t unmatchedSize)
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

  class UnmatchedDetectionResult : public DetectionResult
  {
  public:
    UnmatchedDetectionResult(Binary::Format::Ptr format, Binary::Container::Ptr data)
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
      return Format->Search(RawData->Data(), RawData->Size());
    }
  private:
    const Binary::Format::Ptr Format;
    const Binary::Container::Ptr RawData;
  };
}

namespace ZXTune
{
  DetectionResult::Ptr DetectionResult::CreateMatched(std::size_t matchedSize)
  {
    return boost::make_shared<DetectionResultImpl>(matchedSize, 0);
  }

  DetectionResult::Ptr DetectionResult::CreateUnmatched(Binary::Format::Ptr format, Binary::Container::Ptr data)
  {
    return boost::make_shared<UnmatchedDetectionResult>(format, data);
  }

  DetectionResult::Ptr DetectionResult::CreateUnmatched(std::size_t unmatchedSize)
  {
    return boost::make_shared<DetectionResultImpl>(0, unmatchedSize);
  }
}
