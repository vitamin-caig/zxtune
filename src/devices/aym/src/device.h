/*
Abstract:
  AY/YM chips device class

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  Based on sources of UnrealSpeccy by SMT and Xpeccy sources by SamStyle
*/

#pragma once
#ifndef DEVICES_AYM_DEVICE_H_DEFINED
#define DEVICES_AYM_DEVICE_H_DEFINED

//local includes
#include "generators.h"
//library includes
#include <devices/aym/chip.h>

namespace Devices
{
  namespace AYM
  {
    class AYMDevice
    {
    public:
      AYMDevice()
        : Levels()
        , NoiseMask(HIGH_LEVEL)
        , EnvelopeMask(LOW_LEVEL)
      {
      }

      void SetDutyCycle(uint_t value, uint_t mask)
      {
        GenA.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_A) ? value : NO_DUTYCYCLE);
        GenB.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_B) ? value : NO_DUTYCYCLE);
        GenC.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_C) ? value : NO_DUTYCYCLE);
      }

      void SetMixer(uint_t mixer)
      {
        GenA.SetMasked(0 != (mixer & DataChunk::REG_MASK_TONEA));
        GenB.SetMasked(0 != (mixer & DataChunk::REG_MASK_TONEB));
        GenC.SetMasked(0 != (mixer & DataChunk::REG_MASK_TONEC));
        NoiseMask = HIGH_LEVEL;
        if (0 == (mixer & DataChunk::REG_MASK_NOISEA))
        {
          NoiseMask ^= HIGH_LEVEL_A;
        }
        if (0 == (mixer & DataChunk::REG_MASK_NOISEB))
        {
          NoiseMask ^= HIGH_LEVEL_B;
        }
        if (0 == (mixer & DataChunk::REG_MASK_NOISEC))
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

        if (0 != (levelA & DataChunk::REG_MASK_ENV))
        {
          EnvelopeMask = 0x1;
        }
        else
        {
          Levels = ((levelA & DataChunk::REG_MASK_VOL) << 1) + 1;
        }

        if (0 != (levelB & DataChunk::REG_MASK_ENV))
        {
          EnvelopeMask |= 0x100;
        }
        else
        {
          Levels |= (((levelB & DataChunk::REG_MASK_VOL) << 1) + 1) << 8;
        }

        if (0 != (levelC & DataChunk::REG_MASK_ENV))
        {
          EnvelopeMask |= 0x10000;
        }
        else
        {
          Levels |= (((levelC & DataChunk::REG_MASK_VOL) << 1) + 1) << 16;
        }
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
      ToneGenerator GenA;
      ToneGenerator GenB;
      ToneGenerator GenC;
      NoiseGenerator GenN;
      EnvelopeGenerator GenE;
      uint_t Levels;
      uint_t NoiseMask;
      uint_t EnvelopeMask;
    };
  }
}

#endif //DEVICES_AYM_DEVICE_H_DEFINED
