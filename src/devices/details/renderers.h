/**
 *
 * @file
 *
 * @brief  PSG-based renderers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <devices/details/clock_source.h>
#include <sound/chunk.h>
#include <sound/lpfilter.h>

namespace Devices::Details
{
  template<class StampType>
  class Renderer
  {
  public:
    virtual ~Renderer() = default;

    virtual Sound::Chunk Render(StampType tillTime, uint_t samples) = 0;
    virtual void Render(StampType tillTime, Sound::Chunk* result) = 0;
  };

  /*
  Time (1 uS)
  ||||||||||||||||||||||||||||||||||||||||||||||

  PSG (1773400 / 8 Hz, ~5uS)

  |    |    |    |    |    |    |    |    |    |    |    |    |    |

  Sound (44100 Hz, Clock.SamplePeriod = ~22uS)
  |                     |                     |

                    ->|-|<-- Clock.TicksDelta
                      Clock.NextSampleTime -->|

  */

  template<class StampType, class PSGType>
  class BaseRenderer : public Renderer<StampType>
  {
    using FastStamp = typename ClockSource<StampType>::FastStamp;

  public:
    template<class ParameterType>
    BaseRenderer(ClockSource<StampType>& clock, ParameterType& psg)
      : Clock(clock)
      , PSG(psg)
    {}

    Sound::Chunk Render(StampType tillTime, uint_t samples) override
    {
      Sound::Chunk result;
      result.reserve(samples);
      FinishPreviousSample(&result);
      RenderMultipleSamples(samples - 1, &result);
      StartNextSample(FastStamp(tillTime.Get()));
      return result;
    }

    void Render(StampType tillTime, Sound::Chunk* result) override
    {
      const FastStamp end(tillTime.Get());
      if (Clock.HasSamplesBefore(end))
      {
        FinishPreviousSample(result);
        while (Clock.HasSamplesBefore(end))
        {
          RenderSingleSample(result);
        }
      }
      StartNextSample(end);
    }

  private:
    void FinishPreviousSample(Sound::Chunk* target)
    {
      if (const uint_t ticksPassed = Clock.AdvanceTimeToNextSample())
      {
        PSG.Tick(ticksPassed);
      }
      target->push_back(PSG.GetLevels());
      Clock.UpdateNextSampleTime();
    }

    void RenderMultipleSamples(uint_t samples, Sound::Chunk* target)
    {
      for (uint_t count = samples; count != 0; --count)
      {
        const uint_t ticksPassed = Clock.AllocateSample();
        PSG.Tick(ticksPassed);
        target->push_back(PSG.GetLevels());
      }
      Clock.CommitSamples(samples);
    }

    void RenderSingleSample(Sound::Chunk* target)
    {
      const uint_t ticksPassed = Clock.AdvanceSample();
      PSG.Tick(ticksPassed);
      target->push_back(PSG.GetLevels());
    }

    void StartNextSample(FastStamp till)
    {
      if (const uint_t ticksPassed = Clock.AdvanceTime(till))
      {
        PSG.Tick(ticksPassed);
      }
    }

  protected:
    ClockSource<StampType>& Clock;
    PSGType PSG;
  };

  /*
    Simple decimation algorithm without any filtering
  */
  template<class PSGType>
  class LQWrapper
  {
  public:
    explicit LQWrapper(PSGType& delegate)
      : Delegate(delegate)
    {}

    void Tick(uint_t ticks)
    {
      Delegate.Tick(ticks);
    }

    Sound::Sample GetLevels() const
    {
      return Delegate.GetLevels();
    }

  private:
    PSGType& Delegate;
  };

  /*
    Simple decimation with post simple FIR filter (0.5, 0.5)
  */
  template<class PSGType>
  class MQWrapper
  {
  public:
    explicit MQWrapper(PSGType& delegate)
      : Delegate(delegate)
    {}

    void Tick(uint_t ticks)
    {
      Delegate.Tick(ticks);
    }

    Sound::Sample GetLevels() const
    {
      const Sound::Sample curLevel = Delegate.GetLevels();
      return Interpolate(curLevel);
    }

  private:
    Sound::Sample Interpolate(Sound::Sample newLevel) const
    {
      const Sound::Sample out = Average(PrevLevel, newLevel);
      PrevLevel = newLevel;
      return out;
    }

    static Sound::Sample::Type Average(Sound::Sample::WideType first, Sound::Sample::WideType second)
    {
      return static_cast<Sound::Sample::Type>((first + second) / 2);
    }

    static Sound::Sample Average(Sound::Sample first, Sound::Sample second)
    {
      return Sound::Sample(Average(first.Left(), second.Left()), Average(first.Right(), second.Right()));
    }

  private:
    PSGType& Delegate;
    mutable Sound::Sample PrevLevel;
  };

  /*
    Decimation is performed after 2-order IIR LPF
    Cutoff freq of LPF should be less than Nyquist frequency of target signal
  */
  const uint_t SOUND_CUTOFF_FREQUENCY = 9500;

  template<class PSGType>
  class HQWrapper
  {
  public:
    explicit HQWrapper(PSGType& delegate)
      : Delegate(delegate)
    {}

    void SetClockFrequency(uint64_t clockFreq)
    {
      Filter.SetParameters(clockFreq, SOUND_CUTOFF_FREQUENCY);
    }

    void Tick(uint_t ticksPassed)
    {
      while (ticksPassed--)
      {
        Filter.Feed(Delegate.GetLevels());
        Delegate.Tick(1);
      }
    }

    Sound::Sample GetLevels() const
    {
      return Filter.Get();
    }

  private:
    PSGType& Delegate;
    Sound::LPFilter Filter;
  };

  template<class StampType, class PSGType>
  class LQRenderer : public BaseRenderer<StampType, LQWrapper<PSGType>>
  {
    using Parent = BaseRenderer<StampType, LQWrapper<PSGType>>;

  public:
    LQRenderer(ClockSource<StampType>& clock, PSGType& psg)
      : Parent(clock, psg)
    {}
  };

  template<class StampType, class PSGType>
  class MQRenderer : public BaseRenderer<StampType, MQWrapper<PSGType>>
  {
    using Parent = BaseRenderer<StampType, MQWrapper<PSGType>>;

  public:
    MQRenderer(ClockSource<StampType>& clock, PSGType& psg)
      : Parent(clock, psg)
    {}

  private:
  };

  template<class StampType, class PSGType>
  class HQRenderer : public BaseRenderer<StampType, HQWrapper<PSGType>>
  {
    using Parent = BaseRenderer<StampType, HQWrapper<PSGType>>;

  public:
    HQRenderer(ClockSource<StampType>& clock, PSGType& psg)
      : Parent(clock, psg)
    {}

    void SetClockFrequency(uint64_t clockFreq)
    {
      Parent::PSG.SetClockFrequency(clockFreq);
    }
  };
}  // namespace Devices::Details
