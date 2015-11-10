/**
* 
* @file
*
* @brief  AY/YM device bus implementation
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "device.h"
#include "volume_table.h"

namespace Devices
{
namespace AYM
{
  class PSG
  {
  public:
    explicit PSG(const MultiVolumeTable& table)
      : Table(table)
      , Regs()
    {
      Reset();
    }

    void SetDutyCycle(uint_t value, uint_t mask)
    {
      Device.SetDutyCycle(value, mask);
    }

    void Reset()
    {
      Device.Reset();
      Registers regs;
      for (uint_t idx = 0; idx < Registers::TOTAL; ++idx)
      {
        regs[static_cast<Registers::Index>(idx)] = 0;
      }
      SetNewData(regs);
    }

    void SetNewData(const Registers& data)
    {
      const uint_t REGS_4BIT_SET = (1 << Registers::TONEA_H) | (1 << Registers::TONEB_H) |
        (1 << Registers::TONEC_H) | (1 << Registers::ENV);
      const uint_t REGS_5BIT_SET = (1 << Registers::TONEN) | (1 << Registers::VOLA) |
        (1 << Registers::VOLB) | (1 << Registers::VOLC);

      uint_t used = 0;
      for (Registers::IndicesIterator it(data); it; ++it)
      {
        const Registers::Index reg = *it;
        if (!data.Has(reg))
        {
          //no new data
          continue;
        }
        //copy registers
        uint8_t val = data[reg];
        const uint_t mask = 1 << reg;
        //limit values
        if (mask & REGS_4BIT_SET)
        {
          val &= 0x0f;
        }
        else if (mask & REGS_5BIT_SET)
        {
          val &= 0x1f;
        }
        Regs[reg] = val;
        used |= mask;
      }
      if (used & (1 << Registers::MIXER))
      {
        Device.SetMixer(GetMixer());
      }
      if (used & ((1 << Registers::TONEA_L) | (1 << Registers::TONEA_H)))
      {
        Device.SetToneA(GetToneA());
      }
      if (used & ((1 << Registers::TONEB_L) | (1 << Registers::TONEB_H)))
      {
        Device.SetToneB(GetToneB());
      }
      if (used & ((1 << Registers::TONEC_L) | (1 << Registers::TONEC_H)))
      {
        Device.SetToneC(GetToneC());
      }
      if (used & (1 << Registers::TONEN))
      {
        Device.SetToneN(GetToneN());
      }
      if (used & ((1 << Registers::TONEE_L) | (1 << Registers::TONEE_H)))
      {
        Device.SetToneE(GetToneE());
      }
      if (used & (1 << Registers::ENV))
      {
        Device.SetEnvType(GetEnvType());
      }
      if (used & ((1 << Registers::VOLA) | (1 << Registers::VOLB) | (1 << Registers::VOLC)))
      {
        Device.SetLevel(Regs[Registers::VOLA], Regs[Registers::VOLB], Regs[Registers::VOLC]);
      }
    }

    void Tick(uint_t ticks)
    {
      Device.Tick(ticks);
    }

    Sound::Sample GetLevels() const
    {
      return Table.Get(Device.GetLevels());
    }

    void GetState(MultiChannelState& state) const
    {
      const uint_t TONE_VOICES = 3;
      const LevelType COMMON_LEVEL_DELTA(1, TONE_VOICES);
      const LevelType EMPTY_LEVEL;
      LevelType noiseLevel, envLevel;
      //taking into account only periodic envelope
      const bool periodicEnv = 0 != ((1 << GetEnvType()) & ((1 << 8) | (1 << 10) | (1 << 12) | (1 << 14)));
      const uint_t mixer = ~GetMixer();
      for (uint_t chan = 0; chan != TONE_VOICES; ++chan) 
      {
        const uint_t volReg = Regs[Registers::VOLA + chan];
        const bool hasNoise = 0 != (mixer & (uint_t(Registers::MASK_NOISEA) << chan));
        const bool hasTone = 0 != (mixer & (uint_t(Registers::MASK_TONEA) << chan));
        const bool hasEnv = 0 != (volReg & Registers::MASK_ENV);
        //accumulate level in noise channel
        if (hasNoise)
        {
          noiseLevel += COMMON_LEVEL_DELTA;
        }
        //accumulate level in envelope channel      
        if (hasEnv)
        {        
          envLevel += COMMON_LEVEL_DELTA;
        }
        //calculate tone channel
        if (hasTone)
        {
          const uint_t MAX_VOL = Registers::MASK_VOL;
          const LevelType level(volReg & Registers::MASK_VOL, MAX_VOL);
          const uint_t band = 2 * (256 * Regs[Registers::TONEA_H + chan * 2] +
            Regs[Registers::TONEA_L + chan * 2]);
          state.push_back(ChannelState(band, level));
        }
      }
      if (noiseLevel != EMPTY_LEVEL)
      {
        state.push_back(ChannelState(GetToneN(), noiseLevel));
      }
      if (periodicEnv && envLevel != EMPTY_LEVEL)
      {
        //periodic envelopes has 32 steps, so multiply period to 32
        state.push_back(ChannelState(32 * GetToneE(), envLevel));
      }  
    }
  private:
    uint_t GetMixer() const
    {
      return Regs[Registers::MIXER];
    }

    uint_t GetToneA() const
    {
      return 256 * Regs[Registers::TONEA_H] + Regs[Registers::TONEA_L];
    }

    uint_t GetToneB() const
    {
      return 256 * Regs[Registers::TONEB_H] + Regs[Registers::TONEB_L];
    }

    uint_t GetToneC() const
    {
      return 256 * Regs[Registers::TONEC_H] + Regs[Registers::TONEC_L];
    }

    uint_t GetToneN() const
    {
      return 2 * Regs[Registers::TONEN];//for optimization
    }

    uint_t GetToneE() const
    {
      return 256 * Regs[Registers::TONEE_H] + Regs[Registers::TONEE_L];
    }

    uint_t GetEnvType() const
    {
      return Regs[Registers::ENV];
    }
  private:
    const MultiVolumeTable& Table;
    //registers state
    boost::array<uint_t, Registers::TOTAL> Regs;
    //device
    AYMDevice Device;
  };
}
}
