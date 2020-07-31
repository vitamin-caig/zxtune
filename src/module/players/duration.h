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
  Time::Seconds GetDefaultDuration(const Parameters::Accessor& params);
}
