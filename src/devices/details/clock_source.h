/**
 *
 * @file
 *
 * @brief  Sound devices clock source
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <math/fixedpoint.h>

namespace Devices::Details
{
  template<class StampType>
  class ClockSource
  {
    using FastFixedPoint = Math::FixedPoint<uint_t, 65536U>;

  public:
    using FixedPoint = Math::FixedPoint<typename StampType::ValueType, 65536U>;
    using FastStamp = FixedPoint;

    ClockSource() = default;

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      SoundFreq = soundFreq;
      NextSampleTime -= SamplePeriod;
      SamplePeriod = FixedPoint(StampType::PER_SECOND, soundFreq);
      SampleFreq = FixedPoint(soundFreq, StampType::PER_SECOND);
      PsgFreq = FixedPoint(clockFreq, StampType::PER_SECOND);
      TicksPerSample = FixedPoint(clockFreq, soundFreq);
      NextSampleTime += SamplePeriod;
    }

    void Reset()
    {
      NextSampleTime = SamplePeriod = SampleFreq = CurPsgTime = PsgFreq = 0;
      TicksDelta = TicksPerSample = 0;
    }

    uint_t SamplesTill(StampType stamp) const
    {
      const FastStamp till(stamp.Get());
      assert(till > NextSampleTime);
      const FastStamp curSampleStart = NextSampleTime - SamplePeriod;
      return static_cast<uint_t>(((till - curSampleStart) * SampleFreq).Round());
    }

    bool HasSamplesBefore(StampType stamp) const
    {
      return HasSamplesBefore(FastStamp(stamp.Get()));
    }

    bool HasSamplesBefore(FastStamp stamp) const
    {
      return NextSampleTime < stamp;
    }

    void UpdateNextSampleTime()
    {
      NextSampleTime += SamplePeriod;
    }

    uint_t AllocateSample()
    {
      TicksDelta += TicksPerSample;
      return RoundTicks();
    }

    void CommitSamples(uint_t count)
    {
      const FixedPoint delta = FixedPoint(StampType::PER_SECOND * count, SoundFreq);
      CurPsgTime += delta;
      NextSampleTime += delta;
    }

    uint_t AdvanceSample()
    {
      const uint_t res = AllocateSample();
      CommitSample();
      return res;
    }

    uint_t AdvanceTimeToNextSample()
    {
      // synchronization point
      const uint_t res = AdvanceTime(NextSampleTime);
      NextSampleTime = CurPsgTime;
      return res;
    }

    uint_t AdvanceTime(FastStamp stamp)
    {
      if (stamp > CurPsgTime)
      {
        TicksDelta += FastFixedPoint(PsgFreq * (stamp - CurPsgTime));
        CurPsgTime = stamp;
        return RoundTicks();
      }
      else
      {
        return 0;
      }
    }

  private:
    void CommitSample()
    {
      CurPsgTime += SamplePeriod;
      NextSampleTime += SamplePeriod;
    }

    uint_t RoundTicks()
    {
      const uint_t res = TicksDelta.Integer();
      TicksDelta = FastFixedPoint(TicksDelta.Fraction(), FastFixedPoint::PRECISION);
      return res;
    }

  private:
    uint_t SoundFreq = 0;
    FastStamp NextSampleTime;
    FixedPoint SamplePeriod;
    FixedPoint SampleFreq;
    FastStamp CurPsgTime;
    FixedPoint PsgFreq;
    // tick error accumulator
    FastFixedPoint TicksDelta;
    //
    FastFixedPoint TicksPerSample;
  };
}  // namespace Devices::Details
