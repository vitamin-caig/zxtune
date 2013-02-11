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
        : LevelA(), LevelB(), LevelC()
        , MaskNoiseA(HIGH_LEVEL), MaskNoiseB(HIGH_LEVEL), MaskNoiseC(HIGH_LEVEL)
        , UseEnvA(LOW_LEVEL), UseEnvB(LOW_LEVEL), UseEnvC(LOW_LEVEL)
        , VolumeTable(&GetAY38910VolTable())
      {
      }

      void SetVolumeTable(const VolTable& table)
      {
        VolumeTable = &table;
      }

      void SetDutyCycle(uint_t value, uint_t mask)
      {
        GenA.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_A) ? value : NO_DUTYCYCLE);
        GenB.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_B) ? value : NO_DUTYCYCLE);
        GenC.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_C) ? value : NO_DUTYCYCLE);
        GenN.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_N) ? value : NO_DUTYCYCLE);
        GenE.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_E) ? value : NO_DUTYCYCLE);
      }

      void SetMixer(uint_t mixer)
      {
        GenA.SetMasked(0 != (mixer & DataChunk::REG_MASK_TONEA));
        GenB.SetMasked(0 != (mixer & DataChunk::REG_MASK_TONEB));
        GenC.SetMasked(0 != (mixer & DataChunk::REG_MASK_TONEC));
        MaskNoiseA = 0 != (mixer & DataChunk::REG_MASK_NOISEA) ? HIGH_LEVEL : LOW_LEVEL;
        MaskNoiseB = 0 != (mixer & DataChunk::REG_MASK_NOISEB) ? HIGH_LEVEL : LOW_LEVEL;
        MaskNoiseC = 0 != (mixer & DataChunk::REG_MASK_NOISEC) ? HIGH_LEVEL : LOW_LEVEL;
        GenN.SetMasked(0 != (MaskNoiseA & MaskNoiseB & MaskNoiseC));
      }

      void SetPeriods(uint_t toneA, uint_t toneB, uint_t toneC, uint_t toneN, uint_t toneE)
      {
        GenA.SetPeriod(toneA);
        GenB.SetPeriod(toneB);
        GenC.SetPeriod(toneC);
        GenN.SetPeriod(toneN);
        GenE.SetPeriod(toneE);
      }

      void SetEnvType(uint_t type)
      {
        GenE.SetType(type);
      }

      void SetLevel(uint_t levelA, uint_t levelB, uint_t levelC)
      {
        UseEnvA = 0 != (levelA & DataChunk::REG_MASK_ENV) ? HIGH_LEVEL : LOW_LEVEL;
        UseEnvB = 0 != (levelB & DataChunk::REG_MASK_ENV) ? HIGH_LEVEL : LOW_LEVEL;
        UseEnvC = 0 != (levelC & DataChunk::REG_MASK_ENV) ? HIGH_LEVEL : LOW_LEVEL;
        GenE.SetEnabled(0 != (UseEnvA | UseEnvB | UseEnvC));
        LevelA = (((levelA & DataChunk::REG_MASK_VOL) << 1) + 1) & ~UseEnvA;
        LevelB = (((levelB & DataChunk::REG_MASK_VOL) << 1) + 1) & ~UseEnvB;
        LevelC = (((levelC & DataChunk::REG_MASK_VOL) << 1) + 1) & ~UseEnvC;
      }

      void Reset()
      {
        GenA.Reset();
        GenB.Reset();
        GenC.Reset();
        GenN.Reset();
        GenE.Reset();
        LevelA = LevelB = LevelC = 0;
        MaskNoiseA = MaskNoiseB = MaskNoiseC = HIGH_LEVEL;
        UseEnvA = UseEnvB = UseEnvC = LOW_LEVEL;
        VolumeTable = &GetAY38910VolTable();
      }

      void Tick(uint_t ticks)
      {
        GenA.Tick(ticks);
        GenB.Tick(ticks);
        GenC.Tick(ticks);
        GenN.Tick(ticks);
        GenE.Tick(ticks);
      }

      void GetLevels(MultiSample& result) const
      {
        const uint_t noiseLevel = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();

        const uint_t outA = ((UseEnvA & envelope) | LevelA) & GenA.GetLevel() & (noiseLevel | MaskNoiseA);
        const uint_t outB = ((UseEnvB & envelope) | LevelB) & GenB.GetLevel() & (noiseLevel | MaskNoiseB);
        const uint_t outC = ((UseEnvC & envelope) | LevelC) & GenC.GetLevel() & (noiseLevel | MaskNoiseC);

        const VolTable& table = *VolumeTable;
        assert(outA < 32 && outB < 32 && outC < 32);
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
    private:
      ToneGenerator GenA;
      ToneGenerator GenB;
      ToneGenerator GenC;
      NoiseGenerator GenN;
      EnvelopeGenerator GenE;
      uint_t LevelA;
      uint_t LevelB;
      uint_t LevelC;
      uint_t MaskNoiseA;
      uint_t MaskNoiseB;
      uint_t MaskNoiseC;
      uint_t UseEnvA;
      uint_t UseEnvB;
      uint_t UseEnvC;
      const VolTable* VolumeTable;
    };
  }
}

#endif //DEVICES_AYM_DEVICE_H_DEFINED
