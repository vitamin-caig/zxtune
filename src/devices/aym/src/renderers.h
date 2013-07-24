/*
Abstract:
  AY/YM chips interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_AYM_RENDERERS_H_DEFINED
#define DEVICES_AYM_RENDERERS_H_DEFINED

//library includes
#include <devices/details/clock_source.h>
#include <sound/chunk_builder.h>
#include <sound/lpfilter.h>

namespace Devices
{
namespace AYM
{
  typedef Details::ClockSource<Stamp> ClockSource;

  inline Sound::Sample::Type Average(Sound::Sample::WideType first, Sound::Sample::WideType second)
  {
    return static_cast<Sound::Sample::Type>((first + second) / 2);
  }

  inline Sound::Sample Average(Sound::Sample first, Sound::Sample second)
  {
    return Sound::Sample(Average(first.Left(), second.Left()), Average(first.Right(), second.Right()));
  }

  class Renderer
  {
  public:
    virtual ~Renderer() {}

    virtual void Render(const Stamp& tillTime, Sound::ChunkBuilder& target) = 0;
  };

  /*
    Simple decimation algorithm without any filtering
  */
  template<class PSGType>
  class LQRenderer : public Renderer
  {
  public:
    LQRenderer(ClockSource& clock, PSGType& psg)
      : Clock(clock)
      , PSG(psg)
    {
    }

    virtual void Render(const Stamp& tillTime, Sound::ChunkBuilder& target)
    {
      for (;;)
      {
        const Stamp& nextSampleTime = Clock.GetNextSampleTime();
        if (!(nextSampleTime < tillTime))
        {
          break;
        }
        else if (const uint_t ticksPassed = Clock.NextTime(nextSampleTime))
        {
          PSG.Tick(ticksPassed);
        }
        RenderNextSample(target);
      }
      if (const uint_t ticksPassed = Clock.NextTime(tillTime))
      {
        PSG.Tick(ticksPassed);
      }
    }
  private:
    void RenderNextSample(Sound::ChunkBuilder& target)
    {
      const Sound::Sample sndLevel = PSG.GetLevels();
      target.Add(sndLevel);
      Clock.NextSample();
    }
  private:
    ClockSource& Clock;
    PSGType& PSG;
  };

  /*
    Simple decimation with post simple FIR filter (0.5, 0.5)
  */
  template<class PSGType>
  class MQRenderer : public Renderer
  {
  public:
    MQRenderer(ClockSource& clock, PSGType& psg)
      : Clock(clock)
      , PSG(psg)
    {
    }

    virtual void Render(const Stamp& tillTime, Sound::ChunkBuilder& target)
    {
      for (;;)
      {
        const Stamp& nextSampleTime = Clock.GetNextSampleTime();
        if (!(nextSampleTime < tillTime))
        {
          break;
        }
        else if (const uint_t ticksPassed = Clock.NextTime(nextSampleTime))
        {
          PSG.Tick(ticksPassed);
        }
        RenderNextSample(target);
      }
      if (const uint_t ticksPassed = Clock.NextTime(tillTime))
      {
        PSG.Tick(ticksPassed);
      }
    }
  private:
    void RenderNextSample(Sound::ChunkBuilder& target)
    {
      const Sound::Sample curLevel = PSG.GetLevels();
      const Sound::Sample sndLevel = Interpolate(curLevel);
      target.Add(sndLevel);
      Clock.NextSample();
    }

    Sound::Sample Interpolate(const Sound::Sample& newLevel)
    {
      const Sound::Sample out = Average(PrevLevel, newLevel);
      PrevLevel = newLevel;
      return out;
    }
  private:
    ClockSource& Clock;
    PSGType& PSG;
    Sound::Sample PrevLevel;
  };

  /*
    Decimation is performed after 2-order IIR LPF
    Cutoff freq of LPF should be less than Nyquist frequency of target signal
  */
  template<class PSGType>
  class HQRenderer : public Renderer
  {
  public:
    HQRenderer(ClockSource& clock, PSGType& psg)
      : Clock(clock)
      , PSG(psg)
    {
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      Filter.SetParameters(clockFreq, soundFreq / 4);
    }

    virtual void Render(const Stamp& tillTime, Sound::ChunkBuilder& target)
    {
      for (;;)
      {
        const Stamp& nextSampleTime = Clock.GetNextSampleTime();
        if (!(nextSampleTime < tillTime))
        {
          break;
        }
        else if (const uint_t ticksPassed = Clock.NextTime(nextSampleTime))
        {
          RenderTicks(ticksPassed);
        }
        RenderNextSample(target);
      }
      if (const uint_t ticksPassed = Clock.NextTime(tillTime))
      {
        RenderTicks(ticksPassed);
      }
    }
  private:
    void RenderTicks(uint_t ticksPassed)
    {
      while (ticksPassed--)
      {
        const Sound::Sample curLevel = PSG.GetLevels();
        Filter.Feed(curLevel);
        PSG.Tick(1);
      }
    }

    void RenderNextSample(Sound::ChunkBuilder& target)
    {
      target.Add(Filter.Get());
      Clock.NextSample();
    }
  private:
    ClockSource& Clock;
    PSGType& PSG;
    Sound::LPFilter Filter;
  };

  template<class PSGType>
  class RenderersSet
  {
  public:
    RenderersSet(ClockSource& clock, PSGType& psg)
      : ClockFreq()
      , SoundFreq()
      , Clock(clock)
      , LQ(clock, psg)
      , MQ(clock, psg)
      , HQ(clock, psg)
      , Current()
    {
    }

    void Reset()
    {
      ClockFreq = 0;
      SoundFreq = 0;
      Current = 0;
      Clock.Reset();
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      if (ClockFreq != clockFreq || SoundFreq != soundFreq)
      {
        Clock.SetFrequency(clockFreq, soundFreq);
        HQ.SetFrequency(clockFreq, soundFreq);
        ClockFreq = clockFreq;
        SoundFreq = soundFreq;
      }
    }

    void SetInterpolation(InterpolationType type)
    {
      switch (type)
      {
      case INTERPOLATION_LQ:
        Current = &MQ;
        break;
      case INTERPOLATION_HQ:
        Current = &HQ;
        break;
      default:
        Current = &LQ;
        break;
      }
    }

    Renderer& Get() const
    {
      return *Current;
    }
  private:
    uint64_t ClockFreq;
    uint_t SoundFreq;
    ClockSource& Clock;
    LQRenderer<PSGType> LQ;
    MQRenderer<PSGType> MQ;
    HQRenderer<PSGType> HQ;
    Renderer* Current;
  };
}
}

#endif //DEVICES_AYM_RENDERERS_H_DEFINED
