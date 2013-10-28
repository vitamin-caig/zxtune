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
#include <devices/details/renderers.h>

namespace Devices
{
namespace AYM
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

    void Render(Stamp tillTime, uint_t samples, Sound::ChunkBuilder& target)
    {
      Current->Render(tillTime, samples, target);
    }

    void Render(Stamp tillTime, Sound::ChunkBuilder& target)
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
}
}

#endif //DEVICES_AYM_RENDERERS_H_DEFINED
