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
#include <devices/details/chunks_cache.h>
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
  BOOST_STATIC_ASSERT(DataChunk::REG_LAST <= 8 * sizeof(uint_t));

  class SAARenderer
  {
  public:
    void Reset()
    {
      Device.Reset();
    }

    void SetNewData(const DataChunk& data)
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
        case DataChunk::REG_LEVEL0:
        case DataChunk::REG_LEVEL1:
        case DataChunk::REG_LEVEL2:
        case DataChunk::REG_LEVEL3:
        case DataChunk::REG_LEVEL4:
        case DataChunk::REG_LEVEL5:
          Device.SetLevel(idx - DataChunk::REG_LEVEL0, LoNibble(val), HiNibble(val));
          break;
        case DataChunk::REG_TONENUMBER0:
        case DataChunk::REG_TONENUMBER1:
        case DataChunk::REG_TONENUMBER2:
        case DataChunk::REG_TONENUMBER3:
        case DataChunk::REG_TONENUMBER4:
        case DataChunk::REG_TONENUMBER5:
          Device.SetToneNumber(idx - DataChunk::REG_TONENUMBER0, val);
          break;
        case DataChunk::REG_TONEOCTAVE01:
          Device.SetToneOctave(0, LoNibble(val));
          Device.SetToneOctave(1, HiNibble(val));
          break;
        case DataChunk::REG_TONEOCTAVE23:
          Device.SetToneOctave(2, LoNibble(val));
          Device.SetToneOctave(3, HiNibble(val));
          break;
        case DataChunk::REG_TONEOCTAVE45:
          Device.SetToneOctave(4, LoNibble(val));
          Device.SetToneOctave(5, HiNibble(val));
          break;
        case DataChunk::REG_TONEMIXER:
          Device.SetToneMixer(val);
          break;
        case DataChunk::REG_NOISEMIXER:
          Device.SetNoiseMixer(val);
          break;
        case DataChunk::REG_NOISECLOCK:
          Device.SetNoiseControl(val);
          break;
        case DataChunk::REG_ENVELOPE0:
        case DataChunk::REG_ENVELOPE1:
          Device.SetEnvelope(idx - DataChunk::REG_ENVELOPE0, val);
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

    void GetState(ChannelsState& state) const
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

    Renderer& Get() const
    {
      return *Current;
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
      BufferedData.Add(src);
    }

    virtual void Flush()
    {
      const Stamp till = BufferedData.GetTillTime();
      if (!(till == Stamp(0)))
      {
        SynchronizeParameters();
        Sound::ChunkBuilder builder;
        builder.Reserve(Clock.SamplesTill(till));
        RenderChunks(builder);
        Target->ApplyData(builder.GetResult());
      }
      Target->Flush();
    }

    virtual void Reset()
    {
      Params.Reset();
      PSG.Reset();
      BufferedData.Reset();
      Renderers.Reset();
    }

    virtual void GetState(ChannelsState& state) const
    {
      PSG.GetState(state);
      for (ChannelsState::iterator it = state.begin(), lim = state.end(); it != lim; ++it)
      {
        it->Band = it->Enabled ? Analyser.GetBandByPeriod(it->Band) : 0;
      }
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

    void RenderChunks(Sound::ChunkBuilder& target)
    {
      Renderer& source = Renderers.Get();
      for (const DataChunk* it = BufferedData.GetBegin(), *lim = BufferedData.GetEnd(); it != lim; ++it)
      {
        const DataChunk& chunk = *it;
        if (Clock.GetCurrentTime() < chunk.TimeStamp)
        {
          source.Render(chunk.TimeStamp, target);
        }
        PSG.SetNewData(chunk);
      }
      BufferedData.Reset();
    }
  private:
    Details::ParametersHelper<ChipParameters> Params;
    const Sound::Receiver::Ptr Target;
    SAARenderer PSG;
    ClockSource Clock;
    Details::ChunksCache<DataChunk, Stamp> BufferedData;
    Details::AnalysisMap Analyser;
    RenderersSet Renderers;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
  {
    return Chip::Ptr(new RegularSAAChip(params, target));
  }
}
}
