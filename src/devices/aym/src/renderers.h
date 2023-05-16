/**
 *
 * @file
 *
 * @brief  AY/YM renderers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <devices/aym/chip.h>
#include <devices/details/renderers.h>

namespace Devices::AYM
{
  typedef Details::ClockSource<Stamp> ClockSource;

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
    {}

    void Reset()
    {
      ClockFreq = 0;
      SoundFreq = 0;
      Current = nullptr;
      Clock.Reset();
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

    void Render(Stamp tillTime, Sound::Chunk* target)
    {
      Current->Render(tillTime, target);
    }

  private:
    uint64_t ClockFreq;
    uint_t SoundFreq;
    ClockSource& Clock;
    Details::LQRenderer<Stamp, PSGType> LQ;
    Details::MQRenderer<Stamp, PSGType> MQ;
    Details::HQRenderer<Stamp, PSGType> HQ;
    Details::Renderer<Stamp>* Current;
  };
}  // namespace Devices::AYM
