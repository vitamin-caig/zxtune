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
#include <time/stamp.h>

namespace Parameters
{
  class Accessor;
}

namespace Module
{
  uint_t GetDurationInFrames(const Parameters::Accessor& params, const String& type);
  
  Time::Seconds GetDuration(const Parameters::Accessor& params, const String& type);
}
