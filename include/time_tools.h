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

    Stamp(const Stamp<T, Resolution>& rh)
      : Value(rh.Value)
    {
    }
    
    template<class T1, T1 OtherResolution>
    Stamp(const Stamp<T1, OtherResolution>& rh)
      : Value(sizeof(T1) >= sizeof(T)
        ? Scale(rh.Get(), OtherResolution, T1(Resolution))
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

  typedef Stamp<uint32_t, 1000> Milliseconds;
  typedef Stamp<uint32_t, 1000000> Microseconds;
  typedef Stamp<uint64_t, UINT64_C(1000000000)> Nanoseconds;

  template<class TimeStamp>
  typename TimeStamp::ValueType GetFrequencyForPeriod(const TimeStamp& ts)
  {
    return TimeStamp::PER_SECOND / ts.Get();
  }

  template<class T, class TimeStamp>
  class Oscillator
  {
  public:
    Oscillator()
      : LastFreqChangeTime()
      , LastFreqChangeTick()
      , Frequency()
      , CurTick()
      , CurTimeCache()
    {
    }

    void Reset()
    {
      LastFreqChangeTime = CurTimeCache = 0;
      Frequency = 0;
      LastFreqChangeTick = CurTick = 0;
    }

    void SetFrequency(T freq)
    {
      if (freq != Frequency)
      {
        LastFreqChangeTime = GetCurrentTime().Get();
        LastFreqChangeTick = GetCurrentTick();
        Frequency = freq;
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
        const typename TimeStamp::ValueType relTime = Scale(relTick, Frequency, TimeStamp::PER_SECOND);
        CurTimeCache = LastFreqChangeTime + relTime;
      }
      return TimeStamp(CurTimeCache);
    }

    T GetTickAtTime(const TimeStamp& time) const
    {
      const typename TimeStamp::ValueType relTime = time.Get() - LastFreqChangeTime;
      const T relTick = Scale(relTime, TimeStamp::PER_SECOND, Frequency);
      return LastFreqChangeTick + relTick;
    }
  private:
    typename TimeStamp::ValueType LastFreqChangeTime;
    T LastFreqChangeTick;
    T Frequency;
    T CurTick;
    mutable typename TimeStamp::ValueType CurTimeCache;
  };

  typedef Oscillator<uint64_t, Microseconds> MicrosecOscillator;
  typedef Oscillator<uint64_t, Nanoseconds> NanosecOscillator;
}

#endif //__TIME_TOOLS_H_DEFINED__
