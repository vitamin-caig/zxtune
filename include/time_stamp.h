/**
*
* @file     time_tools.h
* @brief    Time tools interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __TIME_TOOLS_H_DEFINED__
#define __TIME_TOOLS_H_DEFINED__

//common includes
#include <tools.h>
//std includes
#include <cassert>

namespace Time
{
  const uint_t MILLISECONDS_PER_SECOND = 1000;
  const uint_t MICROSECONDS_PER_SECOND = 1000000;
  const uint64_t NANOSECONDS_PER_SECOND = UINT64_C(1000000000);
  const uint_t SECONDS_PER_MINUTE = 60;
  const uint_t MINUTES_PER_HOUR = 60;

  template<class T, T Resolution>
  class Stamp
  {
  public:
    typedef T ValueType;
    static const T PER_SECOND = Resolution;

    Stamp()
      : Value()
    {
    }

    explicit Stamp(T value)
      : Value(value)
    {
    }

    //from the same type
    Stamp(const Stamp<T, Resolution>& rh)
      : Value(rh.Value)
    {
    }

    bool operator < (const Stamp<T, Resolution>& rh) const
    {
      return Value < rh.Value;
    }
    
    //from the other types
    template<class T1, T1 OtherResolution>
    Stamp(const Stamp<T1, OtherResolution>& rh)
      : Value(sizeof(T1) >= sizeof(T)
        ? static_cast<T>(Scale(rh.Get(), OtherResolution, T1(Resolution)))
        : Scale(T(rh.Get()), T(OtherResolution), Resolution))
    {
    }

    template<class T1, T1 OtherResolution>
    const Stamp<T, Resolution>& operator += (const Stamp<T1, OtherResolution>& rh)
    {
      Value += Stamp<T, Resolution>(rh).Get();
      assert(Resolution >= OtherResolution || !"It's unsafe to add timestamp with lesser resolution");
      return *this;
    }

    template<class T1, T1 OtherResolution>
    bool operator < (const Stamp<T1, OtherResolution>& rh) const
    {
      return Value < Stamp<T, Resolution>(rh).Get();
    }

    T Get() const
    {
      return Value;
    }
  private:
    T Value;
  };

  typedef Stamp<uint32_t, MILLISECONDS_PER_SECOND> Milliseconds;
  typedef Stamp<uint32_t, MICROSECONDS_PER_SECOND> Microseconds;
  typedef Stamp<uint64_t, NANOSECONDS_PER_SECOND> Nanoseconds;

  template<class TimeStamp>
  typename TimeStamp::ValueType GetFrequencyForPeriod(const TimeStamp& ts)
  {
    return TimeStamp::PER_SECOND / ts.Get();
  }

  template<class TimeStamp>
  TimeStamp GetPeriodForFrequency(typename TimeStamp::ValueType freq)
  {
    return TimeStamp(TimeStamp::PER_SECOND / freq);
  }
}

#endif //__TIME_TOOLS_H_DEFINED__
