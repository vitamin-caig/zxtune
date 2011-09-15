/*
Abstract:
  Data detection result interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_DETECTION_RESULT_H_DEFINED__
#define __CORE_PLUGINS_DETECTION_RESULT_H_DEFINED__

//library includes
#include <binary/format.h>
#include <io/container.h>

namespace ZXTune
{
  //! Abstract data detection result
  class DetectionResult
  {
  public:
    typedef boost::shared_ptr<const DetectionResult> Ptr;
    virtual ~DetectionResult() {}

    //! @brief Returns data size that is processed in current data position
    //! @return Size of input data format detected
    //! @invariant Return 0 if data is not matched at the beginning
    virtual std::size_t GetMatchedDataSize() const = 0;
    //! @brief Search format in forward data
    //! @return Offset in input data where perform next checking
    //! @invariant Return 0 if current data matches format
    //! @invariant Return input data size if no data at all
    virtual std::size_t GetLookaheadOffset() const = 0;

    static Ptr CreateMatched(std::size_t matchedSize);
    static Ptr CreateUnmatched(Binary::Format::Ptr format, IO::DataContainer::Ptr data);
    static Ptr CreateUnmatched(std::size_t unmatchedSize);
  };
}

#endif //__CORE_PLUGINS_DETECTION_RESULT_H_DEFINED__
