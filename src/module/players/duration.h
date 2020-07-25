/**
* 
* @file
*
* @brief  Duration info
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <time/duration.h>

namespace Parameters
{
  class Accessor;
}

namespace Module
{
  uint_t GetDurationInFrames(const Parameters::Accessor& params);
  
  Time::Seconds GetDuration(const Parameters::Accessor& params);
}
