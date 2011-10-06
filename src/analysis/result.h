/**
*
* @file     analysis/result.h
* @brief    Interface for analyzed result
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ANALYSIS_RESULT_H_DEFINED
#define ANALYSIS_RESULT_H_DEFINED

//library includes
#include <binary/format.h>
#include <binary/container.h>

namespace Analysis
{
  //! Abstract data detection result
  class Result
  {
  public:
    typedef boost::shared_ptr<const Result> Ptr;
    virtual ~Result() {}

    //! @brief Returns data size that is processed in current data position
    //! @return Size of input data format detected
    //! @invariant Return 0 if data is not matched at the beginning
    virtual std::size_t GetMatchedDataSize() const = 0;
    //! @brief Search format in forward data
    //! @return Offset in input data where perform next checking
    //! @invariant Return 0 if current data matches format
    //! @invariant Return input data size if no data at all
    virtual std::size_t GetLookaheadOffset() const = 0;
  };


  Result::Ptr CreateMatchedResult(std::size_t matchedSize);
  Result::Ptr CreateUnmatchedResult(Binary::Format::Ptr format, Binary::Container::Ptr data);
  Result::Ptr CreateUnmatchedResult(std::size_t unmatchedSize);
}

#endif //ANALYSIS_RESULT_H_DEFINED
