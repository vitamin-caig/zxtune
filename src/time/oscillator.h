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
#include <time/stamp.h>

namespace Time
{
  template<class T, class TimeStamp>
  class Oscillator
  {
  public:
    Oscillator()
      : LastFreqChangeTime()
      , LastFreqChangeTick()
      , Frequency()
      , CurTick()
      , ScaleToTime(0, TimeStamp::PER_SECOND)
      , ScaleToTick(TimeStamp::PER_SECOND, 0)
      , CurTimeCache()
    {
    }

    void Reset()
    {
      LastFreqChangeTime = CurTimeCache = 0;
      Frequency = 0;
      ScaleToTime = Math::ScaleFunctor<typename TimeStamp::ValueType>(0, TimeStamp::PER_SECOND);
      ScaleToTick = Math::ScaleFunctor<typename TimeStamp::ValueType>(TimeStamp::PER_SECOND, 0);
      LastFreqChangeTick = CurTick = 0;
    }

    void SetFrequency(T freq)
    {
      if (freq != Frequency)
      {
        LastFreqChangeTime = GetCurrentTime().Get();
        LastFreqChangeTick = GetCurrentTick();
        Frequency = freq;
        ScaleToTime = Math::ScaleFunctor<typename TimeStamp::ValueType>(Frequency, TimeStamp::PER_SECOND);
        ScaleToTick = Math::ScaleFunctor<typename TimeStamp::ValueType>(TimeStamp::PER_SECOND, Frequency);
      }
    }

    void AdvanceTick(T delta)
    {
      CurTick += delta;
      CurTimeCache = 0;
    }

    T GetCurrentTick() const
    {
      return CurTick;
    }

    TimeStamp GetCurrentTime() const
    {
      if (!CurTimeCache && CurTick)
      {
        const T relTick = CurTick - LastFreqChangeTick;
        const typename TimeStamp::ValueType relTime = ScaleToTime(relTick);
        CurTimeCache = LastFreqChangeTime + relTime;
      }
      return TimeStamp(CurTimeCache);
    }

    T GetTickAtTime(const TimeStamp& time) const
    {
      const typename TimeStamp::ValueType relTime = time.Get() - LastFreqChangeTime;
      const T relTick = ScaleToTick(relTime);
      return LastFreqChangeTick + relTick;
    }
  private:
    typename TimeStamp::ValueType LastFreqChangeTime;
    T LastFreqChangeTick;
    T Frequency;
    T CurTick;
    Math::ScaleFunctor<typename TimeStamp::ValueType> ScaleToTime;
    Math::ScaleFunctor<typename TimeStamp::ValueType> ScaleToTick;
    mutable typename TimeStamp::ValueType CurTimeCache;
  };

  typedef Oscillator<uint64_t, Microseconds> MicrosecOscillator;
  typedef Oscillator<uint64_t, Nanoseconds> NanosecOscillator;
}

#endif //TIME_OSCILLATOR_H_DEFINED
