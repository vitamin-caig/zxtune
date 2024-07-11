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

// local includes
#include "device.h"
#include "volume_table.h"
// std includes
#include <array>

namespace Devices::AYM
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
      const uint_t REGS_4BIT_SET = (1 << Registers::TONEA_H) | (1 << Registers::TONEB_H) | (1 << Registers::TONEC_H)
                                   | (1 << Registers::ENV);
      const uint_t REGS_5BIT_SET = (1 << Registers::TONEN) | (1 << Registers::VOLA) | (1 << Registers::VOLB)
                                   | (1 << Registers::VOLC);

      uint_t used = 0;
      for (Registers::IndicesIterator it(data); it; ++it)
      {
        const Registers::Index reg = *it;
        if (!data.Has(reg))
        {
          // no new data
          continue;
        }
        // copy registers
        uint8_t val = data[reg];
        const uint_t mask = 1 << reg;
        // limit values
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
      return 2 * std::max<uint_t>(1, Regs[Registers::TONEN]);  // for optimization
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
    // registers state
    std::array<uint_t, Registers::TOTAL> Regs;
    // device
    AYMDevice Device;
  };
}  // namespace Devices::AYM
