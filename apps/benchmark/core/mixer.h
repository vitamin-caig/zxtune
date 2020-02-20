/**
* 
* @file
*
* @brief  Mixer test interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <time/duration.h>

namespace Benchmark
{
  namespace Mixer
  {
    double Test(uint_t channels, const Time::Milliseconds& duration, uint_t soundFreq);
  }
}
