/*
Abstract:
  Z80 interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __DEVICES_Z80_H_DEFINED__
#define __DEVICES_Z80_H_DEFINED__

//common includes
#include <types.h>
#include <time_oscillator.h>
//boost includes
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>

namespace Devices
{
  namespace Z80
  {
    class ChipIO
    {
    public:
      typedef boost::shared_ptr<ChipIO> Ptr;
      virtual ~ChipIO() {}

      virtual uint8_t Read(uint16_t addr) = 0;
      virtual void Write(const Time::NanosecOscillator& timeStamp, uint16_t addr, uint8_t data) = 0;
    };

    struct Registers
    {
      enum
      {
        REG_AF,
        REG_BC,
        REG_DE,
        REG_HL,
        REG_AF_,
        REG_BC_,
        REG_DE_,
        REG_HL_,
        REG_IX,
        REG_IY,
        REG_IR,
        REG_PC,
        REG_SP,

        REG_LAST
      };

      typedef boost::array<uint16_t, REG_LAST> Dump;
      Dump Data;
      uint32_t Mask;
    };

    class Chip
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;
      virtual ~Chip() {}

      virtual void Reset() = 0;
      virtual void Interrupt() = 0;
      virtual void Execute(const Time::Nanoseconds& till) = 0;
      virtual void SetRegisters(const Registers& regs) = 0;
      virtual void GetRegisters(Registers::Dump& regs) const = 0;
      virtual Time::Nanoseconds GetTime() const = 0;
      virtual uint64_t GetTick() const = 0;
      virtual void SetTime(const Time::Nanoseconds& time) = 0;
    };

    class ChipParameters
    {
    public:
      typedef boost::shared_ptr<const ChipParameters> Ptr;
      virtual ~ChipParameters() {}

      virtual uint_t IntTicks() const = 0;
      virtual uint64_t ClockFreq() const = 0;
    };

    Chip::Ptr CreateChip(ChipParameters::Ptr params, ChipIO::Ptr memory, ChipIO::Ptr ports);
    Chip::Ptr CreateChip(ChipParameters::Ptr params, const Dump& memory, ChipIO::Ptr ports);
  }
}

#endif //__DEVICES_Z80_H_DEFINED__
