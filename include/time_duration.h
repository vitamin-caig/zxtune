/**
*
* @file     time_duration.h
* @brief    Time::Duration itnerface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __TIME_DURATION_H_DEFINED__
#define __TIME_DURATION_H_DEFINED__

//common includes
#include <format.h>
#include <time_stamp.h>

namespace Time
{
  template<class T, class TimeStamp>
  class Duration
  {
  public:
    Duration()
      : Count()
    {
    }

    Duration(T count, const TimeStamp& period)
      : Count(count)
      , Period(period)
    {
    }

    void SetCount(T count)
    {
      Count = count;
    }

    void SetPeriod(const TimeStamp& period)
    {
      Period = period;
    }

    bool operator < (const Duration& rh) const
    {
      return Total() < rh.Total();
    }

    String ToString() const
    {
      const typename TimeStamp::ValueType fpsRough = GetFrequencyForPeriod(Period);
      const typename TimeStamp::ValueType allSeconds = Total() / Period.PER_SECOND;
      const uint_t allMinutes = allSeconds / SECONDS_PER_MINUTE;
      const uint_t frames = Count % fpsRough;
      const uint_t seconds = allSeconds % SECONDS_PER_MINUTE;
      const uint_t minutes = allMinutes % MINUTES_PER_HOUR;
      const uint_t hours = allMinutes / MINUTES_PER_HOUR;
      return Strings::FormatTime(hours, minutes, seconds, frames);
    }
  private:
    typename TimeStamp::ValueType Total() const
    {
      return Period.Get() * Count;
    }
  private:
    T Count;
    TimeStamp Period;
  };

  typedef Duration<uint_t, Milliseconds> MillisecondsDuration;
  typedef Duration<uint_t, Microseconds> MicrosecondsDuration;
  typedef Duration<uint_t, Nanoseconds> NanosecondsDuration;
}

#endif //__TIME_DURATION_H_DEFINED__
