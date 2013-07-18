/*
Abstract:
  AY/YM chips interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_AYM_H_DEFINED
#define DEVICES_AYM_H_DEFINED

//common includes
#include <types.h>
//library includes
#include <time/stamp.h>
//boost includes
#include <boost/array.hpp>

//supporting for AY/YM-based modules
namespace Devices
{
  namespace AYM
  {
    const uint_t VOICES = 5;

    typedef Time::Microseconds Stamp;

    struct DataChunk
    {
      //registers offsets in data
      enum
      {
        REG_TONEA_L,
        REG_TONEA_H,
        REG_TONEB_L,
        REG_TONEB_H,
        REG_TONEC_L,
        REG_TONEC_H,
        REG_TONEN,
        REG_MIXER,
        REG_VOLA,
        REG_VOLB,
        REG_VOLC,
        REG_TONEE_L,
        REG_TONEE_H,
        REG_ENV,
        //limiter
        REG_LAST,
        REG_LAST_AY = REG_ENV + 1,
      };

      //masks
      enum
      {
        //to mark all registers actual
        MASK_ALL_REGISTERS = (1 << REG_LAST) - 1,

        //bits in REG_VOL*
        REG_MASK_VOL = 0x0f,
        REG_MASK_ENV = 0x10,
        //bits in REG_MIXER
        REG_MASK_TONEA = 0x01,
        REG_MASK_NOISEA = 0x08,
        REG_MASK_TONEB = 0x02,
        REG_MASK_NOISEB = 0x10,
        REG_MASK_TONEC = 0x04,
        REG_MASK_NOISEC = 0x20,

        MASK_BEEPER = 1 << REG_LAST,
        MASK_BEEPER_VALUE = 1 << (REG_LAST + 1),
      };

      enum
      {
        CHANNEL_MASK_A = 1,
        CHANNEL_MASK_B = 2,
        CHANNEL_MASK_C = 4,
        CHANNEL_MASK_N = 8,
        CHANNEL_MASK_E = 16,
      };

      class Registers
      {
      public:
        Registers()
          : Mask()
          , Data()
        {
        }

        bool Empty() const
        {
          return 0 == Mask;
        }

        bool Has(uint_t reg) const
        {
          return reg < REG_LAST && 0 != (Mask & (1 << reg));
        }

        uint8_t& operator [] (uint_t reg)
        {
          assert(reg < REG_LAST);
          Mask |= 1 << reg;
          return Data[reg];
        }

        void Reset(uint_t reg)
        {
          assert(reg < REG_LAST);
          Mask &= ~(1 << reg);
        }

        uint8_t operator [] (uint_t reg) const
        {
          assert(Has(reg));
          return Data[reg];
        }

        void SetBeeper(bool val)
        {
          Mask |= val ? (MASK_BEEPER | MASK_BEEPER_VALUE) : MASK_BEEPER;
        }

        bool GetBeeper() const
        {
          assert(HasBeeper());
          return 0 != (Mask & MASK_BEEPER_VALUE);
        }

        bool HasBeeper() const
        {
          return 0 != (Mask & MASK_BEEPER);
        }
      private:
        uint16_t Mask;
        boost::array<uint8_t, REG_LAST> Data;
      };

      DataChunk() : TimeStamp(), Data()
      {
      }

      Stamp TimeStamp;
      Registers Data;
    };

    class Device
    {
    public:
      typedef boost::shared_ptr<Device> Ptr;
      virtual ~Device() {}

      /// render single data chunk
      virtual void RenderData(const DataChunk& src) = 0;

      /// flush any collected data
      virtual void Flush() = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };
  }
}

#endif //DEVICES_AYM_H_DEFINED
