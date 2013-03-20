/**
*
* @file     range_checker.h
* @brief    Range-checking helper interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __RANGE_CHECKER_H_DEFINED__
#define __RANGE_CHECKER_H_DEFINED__

//std includes
#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
//boost includes
#include <boost/array.hpp>

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

  //! @brief Create simple checker with only size limit
  static Ptr CreateSimple(std::size_t limit);
  //! @brief Create checker with upper limit and overlapping possibility check
  static Ptr Create(std::size_t limit);
  //! @brief Create shared checker with upper limit and overlapping possibility check.
  //! @note Supports multiple adding for same range (equal start and size)
  static Ptr CreateShared(std::size_t limit);
};

//! @brief Simple area checker template implementation. Indented for correct format detection
//! @code
//! enum Areas
//! {
//!   HEADER,
//!   CONTENT,
//!   END
//! };
//!
//! AreaController<Areas, 1 + Areas::END> controller;
//! controller.AddArea(HEADER, 0);
//! controller.AddArea(CONTENT, 10);
//! controller.AddArea(END, 20);
//! assert(0 == controller.GetAreaAddress(HEADER));
//! assert(10 == controller.GetAreaSize(HEADER));
//! assert(0 == controller.GetAreaSize(END));
//! @endcode
template<class KeyType, std::size_t AreasCount, class AddrType = std::size_t>
class AreaController
{
  typedef boost::array<AddrType, AreasCount> Area2AddrMap;
public:
  enum
  {
    Undefined = ~AddrType(0)
  };

  AreaController()
  {
    Areas.assign(Undefined);
  }

  void AddArea(KeyType key, AddrType addr)
  {
    assert(Undefined == Areas[key]);
    Areas[key] = addr;
  }

  AddrType GetAreaAddress(KeyType key) const
  {
    const AddrType res = Areas[key];
    return res;
  }

  AddrType GetAreaSize(KeyType key) const
  {
    const AddrType begin = Areas[key];
    if (Undefined == begin)
    {
      //no such area
      return 0;
    }
    AddrType res = Undefined;
    for (std::size_t idx = 0; idx < AreasCount; ++idx)
    {
      const AddrType curAddr = Areas[idx];
      if (Undefined == curAddr ||
          curAddr < begin ||
          idx == key)
      {
        continue;
      }
      const AddrType size = curAddr - begin;
      if (res == Undefined || res > size)
      {
        res = size;
      }
    }
    return res;
  }
private:
  Area2AddrMap Areas;
};

#endif //__RANGE_CHECKER_H_DEFINED__
