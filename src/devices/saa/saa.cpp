/*
Abstract:
  SAA chips interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  Based on sources of PerfectZX emulator
*/

//local includes
#include "device.h"
//common includes
#include <tools.h>
//library includes
#include <devices/details/analysis_map.h>
#include <devices/details/clock_source.h>
#include <devices/details/parameters_helper.h>
#include <sound/chunk_builder.h>
#include <sound/lpfilter.h>
//std includes
#include <cassert>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>

namespace Devices
{
namespace SAA
{
  BOOST_STATIC_ASSERT(Registers::TOTAL <= 8 * sizeof(uint_t));
  BOOST_STATIC_ASSERT(sizeof(Registers) == 32);

  class SAARenderer
  {
  public:
    void Reset()
    {
      Device.Reset();
    }

    void SetNewData(const Registers& data)
    {
      for (uint_t idx = 0, mask = 1; idx != data.Data.size(); ++idx, mask <<= 1)
      {
        if (0 == (data.Mask & mask))
        {
          //no new data
          continue;
        }
        const uint_t val = data.Data[idx];
        switch (idx)
        {
        case Registers::LEVEL0:
        case Registers::LEVEL1:
        case Registers::LEVEL2:
        case Registers::LEVEL3:
        case Registers::LEVEL4:
        case Registers::LEVEL5:
          Device.SetLevel(idx - Registers::LEVEL0, LoNibble(val), HiNibble(val));
          break;
        case Registers::TONENUMBER0:
        case Registers::TONENUMBER1:
        case Registers::TONENUMBER2:
        case Registers::TONENUMBER3:
        case Registers::TONENUMBER4:
        case Registers::TONENUMBER5:
          Device.SetToneNumber(idx - Registers::TONENUMBER0, val);
          break;
        case Registers::TONEOCTAVE01:
          Device.SetToneOctave(0, LoNibble(val));
          Device.SetToneOctave(1, HiNibble(val));
          break;
        case Registers::TONEOCTAVE23:
          Device.SetToneOctave(2, LoNibble(val));
          Device.SetToneOctave(3, HiNibble(val));
          break;
        case Registers::TONEOCTAVE45:
          Device.SetToneOctave(4, LoNibble(val));
          Device.SetToneOctave(5, HiNibble(val));
          break;
        case Registers::TONEMIXER:
          Device.SetToneMixer(val);
          break;
        case Registers::NOISEMIXER:
          Device.SetNoiseMixer(val);
          break;
        case Registers::NOISECLOCK:
          Device.SetNoiseControl(val);
          break;
        case Registers::ENVELOPE0:
        case Registers::ENVELOPE1:
          Device.SetEnvelope(idx - Registers::ENVELOPE0, val);
          break;
        }
      }
    }

    void Tick(uint_t ticks)
    {
      Device.Tick(ticks);
    }

    Sound::Sample GetLevels() const
    {
      return Device.GetLevels();
    }

    void GetState(MultiChannelState& state) const
    {
      Device.GetState(state);
    }
  private:
    SAADevice Device;
  };

  typedef Details::ClockSource<Stamp> ClockSource;

  class Renderer
  {
  public:
    virtual ~Renderer() {}

    virtual void Render(const Stamp& tillTime, Sound::ChunkBuilder& target) = 0;
  };

  /*
    Simple decimation algorithm without any filtering
  */
  class LQRenderer : public Renderer
  {
  public:
    LQRenderer(ClockSource& clock, SAARenderer& psg)
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
      const Sound::Sample& sndLevel = PSG.GetLevels();
      target.Add(sndLevel);
      Clock.NextSample();
    }
  private:
    ClockSource& Clock;
    SAARenderer& PSG;
  };

  /*
    Simple decimation with post simple FIR filter (0.5, 0.5)
  */
  class MQRenderer : public Renderer
  {
  public:
    MQRenderer(ClockSource& clock, SAARenderer& psg)
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
      const Sound::Sample& sndLevel = Interpolate(curLevel);
      target.Add(sndLevel);
      Clock.NextSample();
    }

    Sound::Sample Interpolate(const Sound::Sample& newLevel)
    {
      const Sound::Sample out(Average(PrevLevel.Left(), newLevel.Left()), Average(PrevLevel.Right(), newLevel.Right()));
      PrevLevel = newLevel;
      return out;
    }

    static Sound::Sample::Type Average(Sound::Sample::Type first, Sound::Sample::Type second)
    {
      return static_cast<Sound::Sample::Type>((int_t(first) + second) / 2);
    }
  private:
    ClockSource& Clock;
    SAARenderer& PSG;
    Sound::Sample PrevLevel;
  };

  class HQRenderer : public Renderer
  {
  public:
    HQRenderer(ClockSource& clock, SAARenderer& psg)
      : Clock(clock)
      , PSG(psg)
      , ClockFreq()
      , SoundFreq()
    {
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      if (ClockFreq != clockFreq || SoundFreq != soundFreq)
      {
        Filter.SetParameters(clockFreq / FREQ_DIVIDER, soundFreq / 4);
        ClockFreq = clockFreq;
        SoundFreq = soundFreq;
      }
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
        PSG.Tick(ticksPassed);
      }
    }
  private:
    //minimal period is 512
    static const uint_t FREQ_DIVIDER = 8;

    void RenderTicks(uint_t ticksPassed)
    {
      while (ticksPassed >= FREQ_DIVIDER)
      {
        const Sound::Sample curLevel = PSG.GetLevels();
        Filter.Feed(curLevel);
        PSG.Tick(FREQ_DIVIDER);
        ticksPassed -= FREQ_DIVIDER;
      }
      if (ticksPassed)
      {
        PSG.Tick(ticksPassed);
      }
    }

    void RenderNextSample(Sound::ChunkBuilder& target)
    {
      target.Add(Filter.Get());
      Clock.NextSample();
    }
  private:
    ClockSource& Clock;
    SAARenderer& PSG;
    uint64_t ClockFreq;
    uint_t SoundFreq;
    Sound::LPFilter Filter;
  };

  class RenderersSet
  {
  public:
    RenderersSet(ClockSource& clock, SAARenderer& psg)
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
      Clock.Reset();
      ClockFreq = 0;
      SoundFreq = 0;
      Current = 0;
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

    void Render(const Stamp& tillTime, Sound::ChunkBuilder& target)
    {
      Current->Render(tillTime, target);
    }
  private:
    uint64_t ClockFreq;
    uint_t SoundFreq;
    ClockSource& Clock;
    LQRenderer LQ;
    MQRenderer MQ;
    HQRenderer HQ;
    Renderer* Current;
  };

  class RegularSAAChip : public Chip
  {
  public:
    RegularSAAChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
      : Params(params)
      , Target(target)
      , Clock()
      , Renderers(Clock, PSG)
    {
      RegularSAAChip::Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      if (Clock.GetNextSampleTime() < src.TimeStamp)
      {
        RenderChunksTill(src.TimeStamp);
      }
      PSG.SetNewData(src.Data);
    }

    virtual void Reset()
    {
      Params.Reset();
      PSG.Reset();
      Renderers.Reset();
    }

    virtual void GetState(MultiChannelState& state) const
    {
      MultiChannelState res;
      PSG.GetState(res);
      for (MultiChannelState::iterator it = res.begin(), lim = res.end(); it != lim; ++it)
      {
        it->Band = Analyser.GetBandByPeriod(it->Band);
      }
      state.swap(res);
    }
  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        const uint64_t clock = Params->ClockFreq();
        const uint_t sndFreq = Params->SoundFreq();
        Renderers.SetFrequency(clock, sndFreq);
        Renderers.SetInterpolation(Params->Interpolation());
        Analyser.SetClockRate(clock);
      }
    }

    void RenderChunksTill(Stamp stamp)
    {
      SynchronizeParameters();
      const uint_t samples = Clock.SamplesTill(stamp);
      Sound::ChunkBuilder builder;
      builder.Reserve(samples);
      Renderers.Render(stamp, builder);
      Target->ApplyData(builder.GetResult());
      Target->Flush();
    }
  private:
    Details::ParametersHelper<ChipParameters> Params;
    const Sound::Receiver::Ptr Target;
    SAARenderer PSG;
    ClockSource Clock;
    Details::AnalysisMap Analyser;
    RenderersSet Renderers;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
  {
    return Chip::Ptr(new RegularSAAChip(params, target));
  }
}
}
