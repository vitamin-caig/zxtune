/**
 *
 * @file
 *
 * @brief  Duration serialize function
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "strings/format.h"
#include "time/duration.h"

namespace Time
{
  template<class Unit>
  String ToString(const Duration<Unit>& val)
  {
    const uint_t SECONDS_PER_MINUTE = 60;
    const uint_t MINUTES_PER_HOUR = 60;

    const uint_t secondsTotal = val.Get() / val.PER_SECOND;
    const uint_t minutesTotal = secondsTotal / SECONDS_PER_MINUTE;
    const uint_t seconds = secondsTotal % SECONDS_PER_MINUTE;
    const uint_t minutes = minutesTotal % MINUTES_PER_HOUR;
    const uint_t hours = minutesTotal / MINUTES_PER_HOUR;
    const uint_t centiSeconds = uint_t(val.Get() % val.PER_SECOND) * 100 / val.PER_SECOND;
    return Strings::FormatTime(hours, minutes, seconds, centiSeconds);
  }
}  // namespace Time
