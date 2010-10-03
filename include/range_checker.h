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

  //! @brief Create simple checker with upper limit
  static Ptr Create(std::size_t limit);
  //! @brief Create shared checker with upper limit.
  //! @note Supports multiple adding for same range (equal start and size)
  static Ptr CreateShared(std::size_t limit);
};

template<class KeyType, std::size_t AreasCount, class AddrType = std::size_t>
class AreaController
{
  enum
  {
    NoArea = ~AddrType(0)
  };
  typedef boost::array<AddrType, AreasCount> Area2AddrMap;
public:
  AreaController()
  {
    Areas.assign(NoArea);
  }

  void AddArea(KeyType key, AddrType addr)
  {
    assert(NoArea == Areas[key]);
    Areas[key] = addr;
  }

  AddrType GetAreaAddress(KeyType key) const
  {
    const AddrType res = Areas[key];
    assert(NoArea != res);
    return res;
  }

  AddrType GetAreaSize(KeyType key) const
  {
    const AddrType begin = Areas[key];
    if (NoArea == begin)
    {
      //no such area
      return 0;
    }
    return FindSize(begin);
  }
private:
  AddrType FindSize(AddrType addr) const
  {
    AddrType res = 0;
    for (typename Area2AddrMap::const_iterator it = Areas.begin(), lim = Areas.end(); it != lim; ++it)
    {
      if (*it <= addr || NoArea == *it)
      {
        continue;
      }
      const AddrType size = *it - addr;
      if (!res || res > size)
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
