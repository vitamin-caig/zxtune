/**
* 
* @file
*
* @brief  Simple elapsed timer
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <time/stamp.h>
//std includes
#include <ctime>

namespace Benchmark
{
  class Timer
  {
  public:
    Timer()
      : Start(std::clock())
    {
    }

    template<class Stamp>
    Stamp Elapsed() const
    {
      const std::clock_t elapsed = std::clock() - Start;
      return Stamp(Stamp::PER_SECOND * elapsed / CLOCKS_PER_SEC);
    }
  private:
    const std::clock_t Start;
  };
}
