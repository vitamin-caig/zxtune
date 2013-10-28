/**
*
* @file     oscillator.h
* @brief    Oscillator interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef TIME_OSCILLATOR_H_DEFINED
#define TIME_OSCILLATOR_H_DEFINED

//library includes
#include <math/fixedpoint.h>
#include <time/stamp.h>

namespace Time
{
  template<class TimeStamp, class Tick = typename TimeStamp::ValueType, unsigned Precision = 65536>
  class Oscillator
  {
    typedef Math::FixedPoint<Tick, Precision> FixedPoint;
  public:
    Oscillator()
      : CurTick()
    {
    }

    void Reset()
    {
      CurTick = 0;
      CurTime = 0;
      TimeSlicesPerTick = 0;
      TicksPerTimeSlice = 0;
    }

    void SetFrequency(Tick freq)
    {
      TimeSlicesPerTick = FixedPoint(TimeStamp::PER_SECOND, freq);
      TicksPerTimeSlice = FixedPoint(freq, TimeStamp::PER_SECOND);
    }

    void AdvanceTick()
    {
      ++CurTick;
      CurTime += TimeSlicesPerTick;
    }

    void AdvanceTick(Tick delta)
    {
      CurTick += delta;
      CurTime += TimeSlicesPerTick * delta;
    }

    Tick GetCurrentTick() const
    {
      return CurTick;
    }

    TimeStamp GetCurrentTime() const
    {
      return TimeStamp(CurTime.Integer());
    }

    Tick GetTickAtTime(const TimeStamp& time) const
    {
      FixedPoint relTime(time.Get());
      relTime -= CurTime;
      const Tick relTick = (TicksPerTimeSlice * relTime).Integer();
      return CurTick + relTick;
    }
  private:
    Tick CurTick;
    FixedPoint CurTime;
    FixedPoint TimeSlicesPerTick;
    FixedPoint TicksPerTimeSlice;
  };

  typedef Oscillator<Microseconds> MicrosecOscillator;
  typedef Oscillator<Nanoseconds> NanosecOscillator;

  template<class TimeStamp, class Tick = typename TimeStamp::ValueType, unsigned Precision = 65536>
  class TimedOscillator
  {
    typedef Math::FixedPoint<Tick, Precision> FixedPoint;
  public:
    void Reset()
    {
      CurTime = TimeStamp();
      CurTick = 0;
      TicksPerTimeSlice = 0;
    }

    void SetFrequency(typename FixedPoint::ValueType freq)
    {
      TicksPerTimeSlice = FixedPoint(freq, TimeStamp::PER_SECOND);
    }

    void AdvanceTime(typename TimeStamp::ValueType delta)
    {
      CurTick += TicksPerTimeSlice * delta;
      CurTime = TimeStamp(CurTime.Get() + delta);
    }

    Tick GetCurrentTick() const
    {
      return CurTick.Integer();
    }

    TimeStamp GetCurrentTime() const
    {
      return CurTime;
    }
  private:
    TimeStamp CurTime;
    FixedPoint CurTick;
    FixedPoint TicksPerTimeSlice;
  };
}

#endif //TIME_OSCILLATOR_H_DEFINED
