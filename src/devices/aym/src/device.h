/**
 *
 * @file
 *
 * @brief  AY/YM device implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "generators.h"
// library includes
#include <devices/aym/chip.h>

namespace Devices::AYM
{
  class AYMDevice
  {
  public:
    AYMDevice()
      : NoiseMask(HIGH_LEVEL)
      , EnvelopeMask(LOW_LEVEL)
    {}

    void SetDutyCycle(uint_t value, uint_t mask)
    {
      GenA.SetDutyCycle(0 != (mask & CHANNEL_MASK_A) ? value : NO_DUTYCYCLE);
      GenB.SetDutyCycle(0 != (mask & CHANNEL_MASK_B) ? value : NO_DUTYCYCLE);
      GenC.SetDutyCycle(0 != (mask & CHANNEL_MASK_C) ? value : NO_DUTYCYCLE);
    }

    void SetMixer(uint_t mixer)
    {
      GenA.SetMasked(0 != (mixer & Registers::MASK_TONEA));
      GenB.SetMasked(0 != (mixer & Registers::MASK_TONEB));
      GenC.SetMasked(0 != (mixer & Registers::MASK_TONEC));
      NoiseMask = HIGH_LEVEL;
      if (0 == (mixer & Registers::MASK_NOISEA))
      {
        NoiseMask ^= HIGH_LEVEL_A;
      }
      if (0 == (mixer & Registers::MASK_NOISEB))
      {
        NoiseMask ^= HIGH_LEVEL_B;
      }
      if (0 == (mixer & Registers::MASK_NOISEC))
      {
        NoiseMask ^= HIGH_LEVEL_C;
      }
    }

    void SetToneA(uint_t toneA)
    {
      GenA.SetPeriod(toneA);
    }

    void SetToneB(uint_t toneB)
    {
      GenB.SetPeriod(toneB);
    }

    void SetToneC(uint_t toneC)
    {
      GenC.SetPeriod(toneC);
    }

    void SetToneN(uint_t toneN)
    {
      GenN.SetPeriod(toneN);
    }

    void SetToneE(uint_t toneE)
    {
      GenE.SetPeriod(toneE);
    }

    void SetEnvType(uint_t type)
    {
      GenE.SetType(type);
    }

    void SetLevel(uint_t levelA, uint_t levelB, uint_t levelC)
    {
      Levels = 0;
      EnvelopeMask = 0;

      SetLevel(0, levelA);
      SetLevel(1, levelB);
      SetLevel(2, levelC);
    }

    void Reset()
    {
      GenA.Reset();
      GenB.Reset();
      GenC.Reset();
      GenN.Reset();
      GenE.Reset();
      Levels = 0;
      NoiseMask = HIGH_LEVEL;
      EnvelopeMask = LOW_LEVEL;
    }

    void Tick(uint_t ticks)
    {
      GenA.Tick(ticks);
      GenB.Tick(ticks);
      GenC.Tick(ticks);
      GenN.Tick(ticks);
      GenE.Tick(ticks);
    }

    uint_t GetLevels() const
    {
      const uint_t level = EnvelopeMask ? (EnvelopeMask * GenE.GetLevel()) | Levels : Levels;
      const uint_t noise = NoiseMask != HIGH_LEVEL ? (NoiseMask | GenN.GetLevel()) : NoiseMask;
      const uint_t toneA = GenA.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_A, HIGH_LEVEL>();
      const uint_t toneB = GenB.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_B, HIGH_LEVEL>();
      const uint_t toneC = GenC.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_C, HIGH_LEVEL>();

      return level & toneA & toneB & toneC & noise;
    }

  private:
    void SetLevel(uint_t chan, uint_t reg)
    {
      const uint_t shift = chan * BITS_PER_LEVEL;
      if (0 != (reg & Registers::MASK_ENV))
      {
        EnvelopeMask |= 1 << shift;
      }
      else
      {
        Levels |= ((reg << 1) + 1) << shift;
      }
    }

  private:
    ToneGenerator GenA;
    ToneGenerator GenB;
    ToneGenerator GenC;
    NoiseGenerator GenN;
    EnvelopeGenerator GenE;
    uint_t Levels = 0;
    uint_t NoiseMask;
    uint_t EnvelopeMask;
  };
}  // namespace Devices::AYM
