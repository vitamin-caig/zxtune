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

//std includes
#include <string>
//boost includes
#include <boost/shared_ptr.hpp>

//Text pattern format (in regular expression notation):
//  \?           - any byte
//  [0-9a-fx]{2} - match byte/nibble
//  \+[0-9]+\+   - skip xx bytes (dec)
//  %[01x]{8}    - match byte by bits
//  '.           - match symbol
//  [0-9]{2}-[0-9]{2} - match byte by range

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

#endif //__DETECTOR_H_DEFINED__
