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
//boost includes
#include <boost/mpl/if.hpp>

namespace Time
{
  class Timer
  {
  public:
    //Some of the platforms (e.g. armhf) differs int and long in despite of equal size
    typedef Stamp<boost::mpl::if_c<sizeof(std::clock_t) >= sizeof(uint64_t), uint64_t, uint_t>::type, CLOCKS_PER_SEC> NativeStamp;
  
    Timer()
      : Start(std::clock())
    {
    }
    
    NativeStamp Elapsed() const
    {
      return NativeStamp(std::clock() - Start);
    }
  private:
    //enable assignment
    std::clock_t Start;
  };
}
