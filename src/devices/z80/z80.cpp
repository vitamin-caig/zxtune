/*
Abstract:
  Z80 implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <devices/z80.h>
//3rdparty includes
#include <3rdparty/z80ex/include/z80ex.h>
//boost includes
#include <boost/make_shared.hpp>
//std includes
#include <functional>

namespace
{
  using namespace Devices::Z80;

  class ChipImpl : public Chip
  {
  public:
    ChipImpl(ChipParameters::Ptr params, ChipIO::Ptr memory, ChipIO::Ptr ports)
      : Params(params)
      , Memory(memory)
      , Ports(ports)
      , Context(z80ex_create(&ReadByte, this, &WriteByte, this,
                             &InByte, this, &OutByte, this,
                             &IntRead, this), std::ptr_fun(&z80ex_destroy))
    {
    }

    virtual void Reset()
    {
      z80ex_reset(Context.get());
      Oscillator.Reset();
    }

    virtual void Interrupt()
    {
      Oscillator.SetFrequency(Params->ClockFreq());
      const uint64_t limit = Oscillator.GetCurrentTick() + Params->IntTicks();
      while (Oscillator.GetCurrentTick() < limit)
      {
        if (uint_t tick = z80ex_int(Context.get()))
        {
          Oscillator.AdvanceTick(tick);
          break;
        }
        Oscillator.AdvanceTick(z80ex_step(Context.get()));
      }
    }

    virtual void Execute(const Time::Nanoseconds& till)
    {
      Oscillator.SetFrequency(Params->ClockFreq());
      const uint64_t endTick = Oscillator.GetTickAtTime(till);
      while (Oscillator.GetCurrentTick() < endTick)
      {
        Oscillator.AdvanceTick(z80ex_step(Context.get()));
      }
    }

    virtual void SetState(const Registers& state)
    {
      for (uint_t idx = Registers::REG_AF; idx != Registers::REG_LAST; ++idx)
      {
        if (0 == (state.Mask & (1 << idx)))
        {
          continue;
        }
        const uint16_t value = state.Data[idx];
        switch (idx)
        {
        case Registers::REG_AF:
          z80ex_set_reg(Context.get(), regAF, value);
          break;
        case Registers::REG_BC:
          z80ex_set_reg(Context.get(), regBC, value);
          break;
        case Registers::REG_DE:
          z80ex_set_reg(Context.get(), regDE, value);
          break;
        case Registers::REG_HL:
          z80ex_set_reg(Context.get(), regHL, value);
          break;
        case Registers::REG_AF_:
          z80ex_set_reg(Context.get(), regAF_, value);
          break;
        case Registers::REG_BC_:
          z80ex_set_reg(Context.get(), regBC_, value);
          break;
        case Registers::REG_DE_:
          z80ex_set_reg(Context.get(), regDE_, value);
          break;
        case Registers::REG_HL_:
          z80ex_set_reg(Context.get(), regHL_, value);
          break;
        case Registers::REG_IX:
          z80ex_set_reg(Context.get(), regIX, value);
          break;
        case Registers::REG_IY:
          z80ex_set_reg(Context.get(), regIY, value);
          break;
        case Registers::REG_IR:
          z80ex_set_reg(Context.get(), regI, value >> 8);
          z80ex_set_reg(Context.get(), regR, value & 127);
          z80ex_set_reg(Context.get(), regR7, value & 128);
          break;
        case Registers::REG_PC:
          z80ex_set_reg(Context.get(), regPC, value);
          break;
        case Registers::REG_SP:
          z80ex_set_reg(Context.get(), regSP, value);
          break;
        default:
          assert(!"Invalid register");
        }
      }
    }

    virtual void GetState(Registers::Dump& state) const
    {
      Registers::Dump tmp;
      tmp[Registers::REG_AF] = z80ex_get_reg(Context.get(), regAF);
      tmp[Registers::REG_BC] = z80ex_get_reg(Context.get(), regBC);
      tmp[Registers::REG_DE] = z80ex_get_reg(Context.get(), regDE);
      tmp[Registers::REG_HL] = z80ex_get_reg(Context.get(), regHL);
      tmp[Registers::REG_AF_] = z80ex_get_reg(Context.get(), regAF_);
      tmp[Registers::REG_BC_] = z80ex_get_reg(Context.get(), regBC_);
      tmp[Registers::REG_DE_] = z80ex_get_reg(Context.get(), regDE_);
      tmp[Registers::REG_HL_] = z80ex_get_reg(Context.get(), regHL_);
      tmp[Registers::REG_IX] = z80ex_get_reg(Context.get(), regIX);
      tmp[Registers::REG_IY] = z80ex_get_reg(Context.get(), regIY);
      tmp[Registers::REG_IR] = 256 * z80ex_get_reg(Context.get(), regI) +
        ((z80ex_get_reg(Context.get(), regR) & 127) |
         (z80ex_get_reg(Context.get(), regR7) & 128));
      tmp[Registers::REG_PC] = z80ex_get_reg(Context.get(), regPC);
      tmp[Registers::REG_SP] = z80ex_get_reg(Context.get(), regSP);
      state.swap(tmp);
    }

    virtual Time::Nanoseconds GetTime() const
    {
      return Oscillator.GetCurrentTime();
    }

    virtual uint64_t GetTick() const
    {
      return Oscillator.GetCurrentTick();
    }
  private:
    static Z80EX_BYTE ReadByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD addr, int /*m1_state*/, void* userData)
    {
      ChipImpl* const self = static_cast<ChipImpl*>(userData);
      return self->Read(*self->Memory, addr);
    }

    static void WriteByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD addr, Z80EX_BYTE value, void* userData)
    {
      ChipImpl* const self = static_cast<ChipImpl*>(userData);
      return self->Write(*self->Memory, addr, value);
    }

    static Z80EX_BYTE InByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD port, void* userData)
    {
      ChipImpl* const self = static_cast<ChipImpl*>(userData);
      return self->Read(*self->Ports, port);
    }

    static void OutByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD port, Z80EX_BYTE value, void* userData)
    {
      ChipImpl* const self = static_cast<ChipImpl*>(userData);
      return self->Write(*self->Ports, port, value);
    }

    static Z80EX_BYTE IntRead(Z80EX_CONTEXT* /*cpu*/, void* /*userData*/)
    {
      return 0xff;
    }

    Z80EX_BYTE Read(ChipIO& io, Z80EX_WORD addr)
    {
      return io.Read(addr);
    }

    void Write(ChipIO& io, Z80EX_WORD addr, Z80EX_BYTE value)
    {
      const Time::Nanoseconds time = Oscillator.GetCurrentTime();
      return io.Write(time, addr, value);
    }
  private:
    const ChipParameters::Ptr Params;
    const ChipIO::Ptr Memory;
    const ChipIO::Ptr Ports;
    const boost::shared_ptr<Z80EX_CONTEXT> Context;
    Time::NanosecOscillator Oscillator;
  };
}

namespace Devices
{
  namespace Z80
  {
    Chip::Ptr CreateChip(ChipParameters::Ptr params, ChipIO::Ptr memory, ChipIO::Ptr ports)
    {
      return boost::make_shared<ChipImpl>(params, memory, ports);
    }
  }
}
