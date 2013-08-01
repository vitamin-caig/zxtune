/*
Abstract:
  PSG-based renderers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_DETAILS_RENDERERS_H_DEFINED
#define DEVICES_DETAILS_RENDERERS_H_DEFINED

//library includes
#include <devices/details/clock_source.h>
#include <sound/chunk_builder.h>
#include <sound/lpfilter.h>

namespace Devices
{
namespace Details
{
  template<class StampType>
  class Renderer
  {
  public:
    virtual ~Renderer() {}

    virtual void Render(StampType tillTime, uint_t samples, Sound::ChunkBuilder& target) = 0;
    virtual void Render(StampType tillTime, Sound::ChunkBuilder& target) = 0;
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

  template<class StampType, class PSGType, class Ancestor>
  class BaseRenderer : public Renderer<StampType>
  {
    typedef typename ClockSource<StampType>::FastStamp FastStamp;
  public:
    BaseRenderer(ClockSource<StampType>& clock, PSGType& psg)
      : Clock(clock)
      , PSG(psg)
    {
    }

    virtual void Render(StampType tillTime, uint_t samples, Sound::ChunkBuilder& target)
    {
      FinishPreviousSample(target);
      RenderMultipleSamples(samples - 1, target);
      StartNextSample(FastStamp(tillTime.Get()));
    }

    virtual void Render(StampType tillTime, Sound::ChunkBuilder& target)
    {
      const FastStamp end(tillTime.Get());
      if (Clock.HasSamplesBefore(end))
      {
        FinishPreviousSample(target);
        while (Clock.HasSamplesBefore(end))
        {
          RenderSingleSample(target);
        }
      }
      StartNextSample(end);
    }
  private:
    void FinishPreviousSample(Sound::ChunkBuilder& target)
    {
      if (const uint_t ticksPassed = Clock.AdvanceTimeToNextSample())
      {
        GetAncestor().SkipTicks(ticksPassed);
      }
      GetAncestor().RenderSample(target);
      Clock.UpdateNextSampleTime();
    }

    void RenderMultipleSamples(uint_t samples, Sound::ChunkBuilder& target)
    {
      for (uint_t count = samples; count != 0; --count)
      {
        const uint_t ticksPassed = Clock.AllocateSample();
        GetAncestor().SkipTicks(ticksPassed);
        GetAncestor().RenderSample(target);
      }
      Clock.CommitSamples(samples);
    }

    void RenderSingleSample(Sound::ChunkBuilder& target)
    {
      const uint_t ticksPassed = Clock.AdvanceSample();
      GetAncestor().SkipTicks(ticksPassed);
      GetAncestor().RenderSample(target);
    }

    void StartNextSample(FastStamp till)
    {
      if (const uint_t ticksPassed = Clock.AdvanceTime(till))
      {
        GetAncestor().SkipTicks(ticksPassed);
      }
    }

    Ancestor& GetAncestor()
    {
      return *static_cast<Ancestor*>(this);
    }
  protected:
    ClockSource<StampType>& Clock;
    PSGType& PSG;
  };

  /*
    Simple decimation algorithm without any filtering
  */
  template<class StampType, class PSGType>
  class LQRenderer : public BaseRenderer<StampType, PSGType, LQRenderer<StampType, PSGType> >
  {
    typedef BaseRenderer<StampType, PSGType, LQRenderer> Parent;
  public:
    LQRenderer(ClockSource<StampType>& clock, PSGType& psg)
      : Parent(clock, psg)
    {
    }

  private:
    friend Parent;

    void SkipTicks(uint_t ticks)
    {
      Parent::PSG.Tick(ticks);
    }

    void RenderSample(Sound::ChunkBuilder& target)
    {
      const Sound::Sample sndLevel = Parent::PSG.GetLevels();
      target.Add(sndLevel);
    }
  };

  /*
    Simple decimation with post simple FIR filter (0.5, 0.5)
  */
  template<class StampType, class PSGType>
  class MQRenderer : public BaseRenderer<StampType, PSGType, MQRenderer<StampType, PSGType> >
  {
    typedef BaseRenderer<StampType, PSGType, MQRenderer> Parent;
  public:
    MQRenderer(ClockSource<StampType>& clock, PSGType& psg)
      : Parent(clock, psg)
    {
    }

  private:
    friend Parent;

    void SkipTicks(uint_t ticks)
    {
      Parent::PSG.Tick(ticks);
    }

    void RenderSample(Sound::ChunkBuilder& target)
    {
      const Sound::Sample curLevel = Parent::PSG.GetLevels();
      const Sound::Sample sndLevel = Interpolate(curLevel);
      target.Add(sndLevel);
    }
  
    Sound::Sample Interpolate(const Sound::Sample& newLevel)
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
    Sound::Sample PrevLevel;
  };

  /*
    Decimation is performed after 2-order IIR LPF
    Cutoff freq of LPF should be less than Nyquist frequency of target signal
  */
  template<class StampType, class PSGType>
  class HQRenderer : public BaseRenderer<StampType, PSGType, HQRenderer<StampType, PSGType> >
  {
    typedef BaseRenderer<StampType, PSGType, HQRenderer> Parent;
  public:
    HQRenderer(ClockSource<StampType>& clock, PSGType& psg)
      : Parent(clock, psg)
    {
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      Filter.SetParameters(clockFreq, soundFreq / 4);
    }
  private:
    friend Parent;

    void SkipTicks(uint_t ticksPassed)
    {
      while (ticksPassed--)
      {
        const Sound::Sample curLevel = Parent::PSG.GetLevels();
        Filter.Feed(curLevel);
        Parent::PSG.Tick(1);
      }
    }

    void RenderSample(Sound::ChunkBuilder& target)
    {
      target.Add(Filter.Get());
    }
  private:
    Sound::LPFilter Filter;
  };
}
}

#endif //DEVICES_DETAILS_RENDERERS_H_DEFINED
