/**
 *
 * @file
 *
 * @brief  SAA chips implementation
 *
 * @author vitamin.caig@gmail.com
 *
 * @note Based on sources of PerfectZX emulator
 *
 **/

// local includes
#include "device.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <devices/details/renderers.h>
#include <parameters/tracking_helper.h>
#include <sound/lpfilter.h>
// std includes
#include <cassert>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <utility>

namespace Devices::SAA
{
  static_assert(Registers::TOTAL <= 8 * sizeof(uint_t), "Too many registers for mask");
  static_assert(sizeof(Registers) == 32, "Invalid layout");

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
          // no new data
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

  private:
    SAADevice Device;
  };

  using ClockSource = Details::ClockSource<Stamp>;

  using Renderer = Details::Renderer<Stamp>;
  using LQRenderer = Details::LQRenderer<Stamp, SAARenderer>;
  using MQRenderer = Details::MQRenderer<Stamp, SAARenderer>;

  class HQWrapper
  {
    // minimal period is 512
    static const uint_t FREQ_DIVIDER = 8;

  public:
    explicit HQWrapper(SAARenderer& delegate)
      : Delegate(delegate)
    {}

    void SetClockFrequency(uint64_t clockFreq)
    {
      Filter.SetParameters(clockFreq / FREQ_DIVIDER, Details::SOUND_CUTOFF_FREQUENCY);
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
    using Parent = Details::BaseRenderer<Stamp, HQWrapper>;

  public:
    HQRenderer(ClockSource& clock, SAARenderer& psg)
      : Parent(clock, psg)
    {}

    void SetClockFrequency(uint64_t clockFreq)
    {
      Parent::PSG.SetClockFrequency(clockFreq);
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
    {}

    void Reset()
    {
      Clock.Reset();
      ClockFreq = 0;
      SoundFreq = 0;
      Current = nullptr;
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      if (ClockFreq != clockFreq || SoundFreq != soundFreq)
      {
        Clock.SetFrequency(clockFreq, soundFreq);
        HQ.SetClockFrequency(clockFreq);
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

    Sound::Chunk Render(Stamp tillTime, uint_t samples)
    {
      return Current->Render(tillTime, samples);
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
    explicit RegularSAAChip(ChipParameters::Ptr params)
      : Params(std::move(params))
      , Renderers(Clock, PSG)
    {
      RegularSAAChip::Reset();
    }

    void RenderData(const DataChunk& src) override
    {
      // for simplicity
      Require(!Clock.HasSamplesBefore(src.TimeStamp));
      PSG.SetNewData(src.Data);
    }

    void Reset() override
    {
      Params.Reset();
      PSG.Reset();
      Renderers.Reset();
      SynchronizeParameters();
    }

    Sound::Chunk RenderTill(Stamp stamp) override
    {
      const uint_t samples = Clock.SamplesTill(stamp);
      Require(samples);
      auto result = Renderers.Render(stamp, samples);
      SynchronizeParameters();
      return result;
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
      }
    }

  private:
    Parameters::TrackingHelper<ChipParameters> Params;
    SAARenderer PSG;
    ClockSource Clock;
    RenderersSet Renderers;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params)
  {
    return MakePtr<RegularSAAChip>(std::move(params));
  }
}  // namespace Devices::SAA
