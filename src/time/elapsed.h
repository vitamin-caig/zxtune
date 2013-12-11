/**
*
* @file
*
* @brief  Elapsed time functor helper
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <time/stamp.h>
//std includes
#include <ctime>

namespace Time
{
  class Elapsed
  {
  public:
    template<class StampType>
    explicit Elapsed(const StampType& period)
      : Period(Stamp<typename StampType::ValueType, CLOCKS_PER_SEC>(period).Get())
      , Next()
    {
    }

    bool operator()()
    {
      const clock_t current = ::clock();
      if (Next <= current)
      {
        Next = current + Period;
        return true;
      }
      else
      {
        return false;
      }
    }
  private:
    const clock_t Period;
    clock_t Next;
  };
}
