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
#include <set>
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

template<class KeyType, class AddrType = std::size_t>
class AreaController
{
  struct EntryType
  {
    EntryType()
      : Key(), Addr()
    {
    }

    EntryType(KeyType key, AddrType addr)
      : Key(key), Addr(addr)
    {
    }

    bool operator < (const EntryType& rh) const
    {
      return Addr < rh.Addr;
    }

    bool operator == (KeyType key) const
    {
      return Key == key;
    }

    KeyType Key;
    AddrType Addr;
  };
  typedef std::set<EntryType> EntrySet;
public:
  AreaController()
  {
  }

  void AddArea(KeyType key, AddrType addr)
  {
    assert(Areas.end() == std::find(Areas.begin(), Areas.end(), key));
    Areas.insert(EntryType(key, addr));
  }

  AddrType GetAreaAddress(KeyType key) const
  {
    const typename EntrySet::const_iterator it = FindAreaByKey(key);
    assert(it != Areas.end());
    return it->Addr;
  }

  AddrType GetAreaSize(KeyType key) const
  {
    const typename EntrySet::const_iterator it = FindAreaByKey(key);
    if (it == Areas.end())
    {
      //no such area
      return 0;
    }
    typename EntrySet::const_iterator next = it;
    ++next;
    if (next == Areas.end())
    {
      //last area- unknown size
      return 0;
    }
    return next->Addr - it->Addr;
  }
  private:
   typename EntrySet::const_iterator FindAreaByKey(KeyType key) const
   {
     return std::find(Areas.begin(), Areas.end(), key);
   }
  private:
    EntrySet Areas;
  };


#endif //__RANGE_CHECKER_H_DEFINED__
