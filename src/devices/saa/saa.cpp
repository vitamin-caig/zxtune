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
#include <devices/details/renderers.h>
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

  typedef Details::Renderer<Stamp> Renderer;
  typedef Details::LQRenderer<Stamp, SAARenderer> LQRenderer;
  typedef Details::MQRenderer<Stamp, SAARenderer> MQRenderer;

  class HQWrapper
  {
    //minimal period is 512
    static const uint_t FREQ_DIVIDER = 8;
  public:
    explicit HQWrapper(SAARenderer& delegate)
      : Delegate(delegate)
    {
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      Filter.SetParameters(clockFreq / FREQ_DIVIDER, soundFreq / 4);
    }

    void Tick(uint_t ticksPassed)
    {
      while (ticksPassed >= FREQ_DIVIDER)
      {
        Filter.Feed(Delegate.GetLevels());
        Delegate.Tick(FREQ_DIVIDER);
        ticksPassed -= FREQ_DIVIDER;
      }
      if (ticksPassed)
      {
        Delegate.Tick(ticksPassed);
      }
    }

    Sound::Sample GetLevels() const
    {
      return Filter.Get();
    }
  private:
    SAARenderer& Delegate;
    Sound::LPFilter Filter;
  };   

  class HQRenderer : public Details::BaseRenderer<Stamp, HQWrapper>
  {
    typedef Details::BaseRenderer<Stamp, HQWrapper> Parent;
  public:
    HQRenderer(ClockSource& clock, SAARenderer& psg)
      : Parent(clock, psg)
    {
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      Parent::PSG.SetFrequency(clockFreq, soundFreq);
    }
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

    void Render(Stamp tillTime, uint_t samples, Sound::ChunkBuilder& target)
    {
      Current->Render(tillTime, samples, target);
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
      if (Clock.HasSamplesBefore(src.TimeStamp))
      {
        SynchronizeParameters();
        RenderTill(src.TimeStamp);
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

    void RenderTill(Stamp stamp)
    {
      const uint_t samples = Clock.SamplesTill(stamp);
      Sound::ChunkBuilder builder;
      builder.Reserve(samples);
      Renderers.Render(stamp, samples, builder);
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
