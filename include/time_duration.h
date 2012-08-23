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
//boost includes
#include <boost/math/common_factor_rt.hpp>

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

    template<class T1>
    Duration(const Duration<T1, TimeStamp>& rh)
      : Count(rh.Count)
      , Period(rh.Period)
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

    T GetCount() const
    {
      return Count;
    }

    template<class OtherTimestamp>
    void SetPeriod(const OtherTimestamp& period)
    {
      SetPeriod(TimeStamp(period));
    }

    template<class T1>
    bool operator < (const Duration<T1, TimeStamp>& rh) const
    {
      return Total() < rh.Total();
    }

    template<class T1>
    Duration operator + (const Duration<T1, TimeStamp>& rh) const
    {
      Duration lh(*this);
      return lh += rh;
    }

    template<class T1>
    Duration& operator += (const Duration<T1, TimeStamp>& rh)
    {
      if (Period == rh.Period)
      {
        Count += rh.Count;
      }
      else
      {
        const typename TimeStamp::ValueType newPeriod = boost::math::gcd(Period.Get(), rh.Period.Get());
        const T thisMult = Period.Get() / newPeriod;
        const T rhMult = rh.Period.Get() / newPeriod;
        Count = Count * thisMult + rh.Count * rhMult;
        Period = TimeStamp(newPeriod);
      }
      return *this;
    }

    String ToString() const
    {
      const typename TimeStamp::ValueType allUnits = Total();
      const typename TimeStamp::ValueType allSeconds = allUnits / Period.PER_SECOND;
      const uint_t allMinutes = allSeconds / SECONDS_PER_MINUTE;
      const uint_t units = allUnits % Period.PER_SECOND;
      const uint_t frames = units / Period.Get();
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

    template<typename, typename> friend class Duration;
  private:
    T Count;
    TimeStamp Period;
  };

  typedef Duration<uint_t, Milliseconds> MillisecondsDuration;
  typedef Duration<uint_t, Microseconds> MicrosecondsDuration;
  typedef Duration<uint_t, Nanoseconds> NanosecondsDuration;
}

#endif //__TIME_DURATION_H_DEFINED__
