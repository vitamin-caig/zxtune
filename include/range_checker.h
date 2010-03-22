/*
Abstract:
  Range-checking helper interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __RANGE_CHECKER_H_DEFINED__
#define __RANGE_CHECKER_H_DEFINED__

#include <memory>
#include <utility>

class RangeChecker
{
public:
  typedef std::auto_ptr<RangeChecker> Ptr;
  //start, size
  virtual ~RangeChecker() {}

  virtual bool AddRange(std::size_t offset, std::size_t size) = 0;

  typedef std::pair<std::size_t, std::size_t> Range;
  virtual Range GetAffectedRange() const = 0;

  static Ptr Create(std::size_t limit);
  static Ptr CreateShared(std::size_t limit);
};

#endif //__RANGE_CHECKER_H_DEFINED__
