/**
 *
 * @file
 *
 * @brief  Oscillator interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "math/fixedpoint.h"
#include "time/instant.h"

namespace Time
{
  template<class Unit, class Tick = typename Unit::StorageType, unsigned Precision = 65536>
  class Oscillator
  {
    using FixedPoint = Math::FixedPoint<Tick, Precision>;

  public:
    Oscillator()
      : CurTick()
    {}

    void Reset()
    {
      CurTick = 0;
      CurTime = 0;
      TimeSlicesPerTick = 0;
      TicksPerTimeSlice = 0;
    }

    void SetFrequency(Tick freq)
    {
      TimeSlicesPerTick = FixedPoint(Unit::PER_SECOND, freq);
      TicksPerTimeSlice = FixedPoint(freq, Unit::PER_SECOND);
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

    Instant<Unit> GetCurrentTime() const
    {
      return Instant<Unit>(CurTime.Integer());
    }

    //! Move the timeline to the given instant (never backwards). The tick conversion truncates,
    //! so advance until the integer time actually reaches the target.
    void AdvanceTime(Instant<Unit> time)
    {
      while (GetCurrentTime() < time)
      {
        const auto delta = GetTickAtTime(time) - CurTick;
        AdvanceTick(delta ? delta : 1);
      }
    }

    Tick GetTickAtTime(Instant<Unit> time) const
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

  using MicrosecOscillator = Oscillator<Microsecond>;
}  // namespace Time
