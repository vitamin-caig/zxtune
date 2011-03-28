/**
*
* @file     detector.h
* @brief    Format detector interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __DETECTOR_H_DEFINED__
#define __DETECTOR_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <string>
//boost includes
#include <boost/shared_ptr.hpp>

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

  static Ptr Create(std::size_t matchedSize, std::size_t lookAhead);
};

//Text pattern format:
//  xx - match byte (hex)
//  ? - any byte
//  +xx+ - skip xx bytes (dec)

//! Data format description
class DataFormat
{
public:
  typedef boost::shared_ptr<const DataFormat> Ptr;
  virtual ~DataFormat() {}

  //! @brief Check if input data is data format
  //! @param data Data to be checked
  //! @param size Size of data to be checked
  //! @return true if data comply format
  virtual bool Match(const void* data, std::size_t size) const = 0;
  //! @brief Search for matched offset in input data
  //! @param data Data to be checked
  //! @param size Size of data to be checked
  //! @return Offset of matched data or size if not found
  virtual std::size_t Search(const void* data, std::size_t size) const = 0;

  // Factory based on text pattern
  static Ptr Create(const std::string& pattern);
};

typedef std::vector<uint16_t> BinaryPattern;
bool DetectFormat(const uint8_t* data, std::size_t size, const BinaryPattern& pattern);

void CompileDetectPattern(const std::string& textPattern, BinaryPattern& binPattern);

class DetectFormatHelper
{
public:
  explicit DetectFormatHelper(const std::string& txtPattern)
  {
    CompileDetectPattern(txtPattern, Pattern);
  }

  bool operator ()(const void* data, std::size_t size) const
  {
    return DetectFormat(static_cast<const uint8_t*>(data), size, Pattern);
  }
private:
  BinaryPattern Pattern;
};

#endif //__DETECTOR_H_DEFINED__
