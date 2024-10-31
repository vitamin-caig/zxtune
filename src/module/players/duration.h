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

#include <time/duration.h>

namespace Parameters
{
  class Accessor;
}

namespace Module
{
  Time::Seconds GetDefaultDuration(const Parameters::Accessor& params);
}
