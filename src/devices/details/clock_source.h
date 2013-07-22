/*
Abstract:
  ClockSource helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_DETAILS_CLOCK_SOURCE_H_DEFINED
#define DEVICES_DETAILS_CLOCK_SOURCE_H_DEFINED

//std includes
#include <time/oscillator.h>

namespace Devices
{
  namespace Details
  {
    template<class StampType>
    class ClockSource
    {
    public:
      void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
      {
        SndOscillator.SetFrequency(soundFreq);
        PsgOscillator.SetFrequency(clockFreq);
      }

      void Reset()
      {
        PsgOscillator.Reset();
        SndOscillator.Reset();
      }

      StampType GetCurrentTime() const
      {
        return PsgOscillator.GetCurrentTime();
      }

      StampType GetNextSampleTime() const
      {
        return SndOscillator.GetCurrentTime();
      }

      void NextSample()
      {
        SndOscillator.AdvanceTick();
      }

      uint_t NextTime(const StampType& stamp)
      {
        const StampType prevStamp = PsgOscillator.GetCurrentTime();
        const uint64_t prevTick = PsgOscillator.GetCurrentTick();
        PsgOscillator.AdvanceTime(stamp.Get() - prevStamp.Get());
        return static_cast<uint_t>(PsgOscillator.GetCurrentTick() - prevTick);
      }

      uint_t SamplesTill(const StampType& stamp) const
      {
        //TODO: investigate for adding reason
        return SndOscillator.GetTickAtTime(stamp) - SndOscillator.GetCurrentTick() + 2;
      }
    private:
      Time::Oscillator<StampType> SndOscillator;
      Time::TimedOscillator<StampType> PsgOscillator;
    };
  }
}

#endif //DEVICES_DETAILS_CLOCK_SOURCE_H_DEFINED
