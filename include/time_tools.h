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

namespace Time
{
  //! @brief performs scaling from tick to absolute time and reverse
  template<class T, T TimeResolution>
  class FreqScaler
  {
  public:
    FreqScaler()
      : BaseTime()
      , BaseTick()
      , Frequency()
      , CurTime()
      , CurTick()
    {
    }

    void SetFrequency(T freq)
    {
      if (freq != Frequency)
      {
        BaseTime = CurTime;
        BaseTick = CurTick;
        Frequency = freq;
      }
    }

    void Reset()
    {
      BaseTime = BaseTick = Frequency = CurTime = CurTick = 0;
    }

    void SetTime(T time)
    {
      assert(Frequency);
      assert(time >= CurTime);
      CurTime = time;
      const T relTime = CurTime - BaseTime;
      const T relTick = BigMulDiv(relTime, Frequency, TimeResolution);
      CurTick = BaseTick + relTick;
    }

    void SetTick(T tick)
    {
      assert(Frequency);
      assert(tick >= CurTick);
      CurTick = tick;
      const T relTick = CurTick - BaseTick;
      const T relTime = BigMulDiv(relTick, TimeResolution, Frequency);
      CurTime = BaseTime + relTime;
    }

    void AdviceTick(T delta)
    {
      SetTick(CurTick + delta);
    }

    T GetTime() const
    {
      return CurTime;
    }

    T GetTick() const
    {
      return CurTick;
    }
  private:
    T BaseTime;
    T BaseTick;
    T Frequency;
    T CurTime;
    T CurTick;
  };

  typedef FreqScaler<uint64_t, 1000000> MicrosecFreqScaler;
  typedef FreqScaler<uint64_t, UINT64_C(1000000000)> NanosecFreqScaler;
}

#endif //__TIME_TOOLS_H_DEFINED__
