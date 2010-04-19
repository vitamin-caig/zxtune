/**
*
* @file     range_checker.h
* @brief    Range-checking helper interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __RANGE_CHECKER_H_DEFINED__
#define __RANGE_CHECKER_H_DEFINED__

//std includes
#include <memory>
#include <utility>

//! @brief Memory ranges checker interface. Intented for correct format detection
class RangeChecker
{
public:
  typedef std::auto_ptr<RangeChecker> Ptr;

  virtual ~RangeChecker() {}

  //! @brief Adding range to collection
  //! @param offset 0-based range offset in bytes
  //! @param size Range size
  //! @return true if range is successfully added
  virtual bool AddRange(std::size_t offset, std::size_t size) = 0;

  //! @brief Helper type
  typedef std::pair<std::size_t, std::size_t> Range;
  //! @brief Calculating affected range using
  //! @return Cumulative range
  virtual Range GetAffectedRange() const = 0;

  //! @brief Create simple checker with upper limit
  static Ptr Create(std::size_t limit);
  //! @brief Create shared checker with upper limit.
  //! @note Supports multiple adding for same range (equal start and size)
  static Ptr CreateShared(std::size_t limit);
};

#endif //__RANGE_CHECKER_H_DEFINED__
