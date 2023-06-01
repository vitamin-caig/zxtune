/**
 *
 * @file
 *
 * @brief  Z80 implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include <devices/z80.h>
// common includes
#include <make_ptr.h>
// library includes
#include <binary/dump.h>
#include <parameters/tracking_helper.h>
// 3rdparty includes
#include <3rdparty/z80ex/include/z80ex.h>

namespace Devices::Z80
{
  class IOBus
  {
  public:
    virtual ~IOBus() = default;

    virtual std::shared_ptr<Z80EX_CONTEXT> ConnectCPU() const = 0;
  };

  class ExtendedIOBus : public IOBus
  {
  public:
    ExtendedIOBus(const Oscillator& clock, ChipIO::Ptr memory, ChipIO::Ptr ports)
      : Clock(clock)
      , Memory(std::move(memory))
      , Ports(std::move(ports))
    {}

    std::shared_ptr<Z80EX_CONTEXT> ConnectCPU() const override
    {
      auto* const self = const_cast<ExtendedIOBus*>(this);
      return {z80ex_create(&ReadByte, self, &WriteByte, self, &InByte, self, &OutByte, self, &IntRead, self),
              &z80ex_destroy};
    }

  private:
    static Z80EX_BYTE ReadByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD addr, int /*m1_state*/, void* userData)
    {
      const auto* const self = static_cast<const ExtendedIOBus*>(userData);
      return self->Read(*self->Memory, addr);
    }

    static void WriteByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD addr, Z80EX_BYTE value, void* userData)
    {
      const auto* const self = static_cast<const ExtendedIOBus*>(userData);
      return self->Write(*self->Memory, addr, value);
    }

    static Z80EX_BYTE InByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD port, void* userData)
    {
      const auto* const self = static_cast<const ExtendedIOBus*>(userData);
      return self->Read(*self->Ports, port);
    }

    static void OutByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD port, Z80EX_BYTE value, void* userData)
    {
      const auto* const self = static_cast<const ExtendedIOBus*>(userData);
      return self->Write(*self->Ports, port, value);
    }

    static Z80EX_BYTE IntRead(Z80EX_CONTEXT* /*cpu*/, void* /*userData*/)
    {
      return 0xff;
    }

    static Z80EX_BYTE Read(ChipIO& io, Z80EX_WORD addr)
    {
      return io.Read(addr);
    }

    void Write(ChipIO& io, Z80EX_WORD addr, Z80EX_BYTE value) const
    {
      return io.Write(Clock, addr, value);
    }

  private:
    const Oscillator& Clock;
    const ChipIO::Ptr Memory;
    const ChipIO::Ptr Ports;
  };

  class SimpleIOBus : public IOBus
  {
  public:
    SimpleIOBus(const Oscillator& clock, Binary::View memory, ChipIO::Ptr ports)
      : Clock(clock)
      , Memory(memory.As<uint8_t>(), memory.As<uint8_t>() + memory.Size())
      , RawMemory(Memory.data())
      , Ports(std::move(ports))
    {}

    std::shared_ptr<Z80EX_CONTEXT> ConnectCPU() const override
    {
      auto* const self = const_cast<SimpleIOBus*>(this);
      const bool isLimited = Memory.size() < 65536;
      const z80ex_mread_cb read = isLimited ? &ReadByteLimited : &ReadByteUnlimited;
      const z80ex_mwrite_cb write = isLimited ? &WriteByteLimited : &WriteByteUnlimited;
      return {z80ex_create(read, self, write, self, &InByte, self, &OutByte, self, &IntRead, self), &z80ex_destroy};
    }

  private:
    static Z80EX_BYTE ReadByteUnlimited(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD addr, int /*m1_state*/, void* userData)
    {
      const auto* const self = static_cast<const SimpleIOBus*>(userData);
      return self->RawMemory[addr];
    }

    static Z80EX_BYTE ReadByteLimited(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD addr, int /*m1_state*/, void* userData)
    {
      const auto* const self = static_cast<const SimpleIOBus*>(userData);
      return addr < self->Memory.size() ? self->RawMemory[addr] : 0xff;
    }

    static void WriteByteUnlimited(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD addr, Z80EX_BYTE value, void* userData)
    {
      auto* const self = static_cast<SimpleIOBus*>(userData);
      self->RawMemory[addr] = value;
    }

    static void WriteByteLimited(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD addr, Z80EX_BYTE value, void* userData)
    {
      auto* const self = static_cast<SimpleIOBus*>(userData);
      if (addr < self->Memory.size())
      {
        self->RawMemory[addr] = value;
      }
    }

    static Z80EX_BYTE InByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD port, void* userData)
    {
      const auto* const self = static_cast<const SimpleIOBus*>(userData);
      return self->Ports->Read(port);
    }

    static void OutByte(Z80EX_CONTEXT* /*cpu*/, Z80EX_WORD port, Z80EX_BYTE value, void* userData)
    {
      const auto* const self = static_cast<const SimpleIOBus*>(userData);
      return self->Ports->Write(self->Clock, port, value);
    }

    static Z80EX_BYTE IntRead(Z80EX_CONTEXT* /*cpu*/, void* /*userData*/)
    {
      return 0xff;
    }

  private:
    const Oscillator& Clock;
    Binary::Dump Memory;
    uint8_t* const RawMemory;
    const ChipIO::Ptr Ports;
  };

  class ClockSource
  {
  public:
    ClockSource() = default;

    void Reset()
    {
      ClockFreq = 0;
      IntDuration = 0;
      Clock.Reset();
    }

    void SetParameters(uint64_t clockFreq, uint_t intDuration)
    {
      if (clockFreq != ClockFreq || intDuration != IntDuration)
      {
        ClockFreq = clockFreq;
        IntDuration = intDuration;
        Clock.SetFrequency(ClockFreq);
      }
    }

    void AdvanceTick(uint_t delta)
    {
      Clock.AdvanceTick(delta);
    }

    void Seek(const Stamp time)
    {
      Clock.Reset();
      const uint64_t tick = Clock.GetTickAtTime(time);
      Clock.SetFrequency(ClockFreq);
      Clock.AdvanceTick(tick);
    }

    const Oscillator& GetOscillator() const
    {
      return Clock;
    }

    uint64_t GetCurrentTick() const
    {
      return Clock.GetCurrentTick();
    }

    Stamp GetCurrentTime() const
    {
      return Clock.GetCurrentTime();
    }

    uint64_t GetTickAtTime(const Stamp till) const
    {
      return Clock.GetTickAtTime(till);
    }

    uint64_t GetIntEnd() const
    {
      return Clock.GetCurrentTick() + IntDuration;
    }

  private:
    uint64_t ClockFreq = 0;
    uint_t IntDuration = 0;
    Oscillator Clock;
  };

  class Z80Chip : public Chip
  {
  public:
    Z80Chip(ChipParameters::Ptr params, ChipIO::Ptr memory, ChipIO::Ptr ports)
      : Params(std::move(params))
      , Bus(new ExtendedIOBus(Clock.GetOscillator(), std::move(memory), std::move(ports)))
      , Context(Bus->ConnectCPU())
    {
      Z80Chip::Reset();
    }

    Z80Chip(ChipParameters::Ptr params, Binary::View memory, ChipIO::Ptr ports)
      : Params(std::move(params))
      , Bus(new SimpleIOBus(Clock.GetOscillator(), memory, std::move(ports)))
      , Context(Bus->ConnectCPU())
    {
      Z80Chip::Reset();
    }

    void Reset() override
    {
      Params.Reset();
      z80ex_reset(Context.get());
      Clock.Reset();
    }

    void Interrupt() override
    {
      SynchronizeParameters();
      const uint64_t limit = Clock.GetIntEnd();
      while (Clock.GetCurrentTick() < limit)
      {
        if (uint_t tick = z80ex_int(Context.get()))
        {
          Clock.AdvanceTick(tick);
          continue;
        }
        Clock.AdvanceTick(z80ex_step(Context.get()));
      }
    }

    void Execute(const Stamp& till) override
    {
      const uint64_t endTick = Clock.GetTickAtTime(till);
      while (Clock.GetCurrentTick() < endTick)
      {
        Clock.AdvanceTick(z80ex_step(Context.get()));
      }
    }

    void SetRegisters(const Registers& regs) override
    {
      for (uint_t idx = Registers::REG_AF; idx != Registers::REG_LAST; ++idx)
      {
        if (0 == (regs.Mask & (1 << idx)))
        {
          continue;
        }
        const uint16_t value = regs.Data[idx];
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

    void GetRegisters(Registers::Dump& regs) const override
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
      tmp[Registers::REG_IR] =
          256 * z80ex_get_reg(Context.get(), regI)
          + ((z80ex_get_reg(Context.get(), regR) & 127) | (z80ex_get_reg(Context.get(), regR7) & 128));
      tmp[Registers::REG_PC] = z80ex_get_reg(Context.get(), regPC);
      tmp[Registers::REG_SP] = z80ex_get_reg(Context.get(), regSP);
      regs.swap(tmp);
    }

    Stamp GetTime() const override
    {
      return Clock.GetCurrentTime();
    }

    uint64_t GetTick() const override
    {
      return Clock.GetCurrentTick();
    }

    void SetTime(const Stamp& time) override
    {
      SynchronizeParameters();
      Clock.Seek(time);
    }

  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        Clock.SetParameters(Params->ClockFreq(), Params->IntTicks());
      }
    }

  private:
    Parameters::TrackingHelper<ChipParameters> Params;
    ClockSource Clock;
    const std::unique_ptr<IOBus> Bus;
    const std::shared_ptr<Z80EX_CONTEXT> Context;
  };
}  // namespace Devices::Z80

namespace Devices::Z80
{
  Chip::Ptr CreateChip(ChipParameters::Ptr params, ChipIO::Ptr memory, ChipIO::Ptr ports)
  {
    return MakePtr<Z80Chip>(std::move(params), std::move(memory), std::move(ports));
  }

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Binary::View memory, ChipIO::Ptr ports)
  {
    return MakePtr<Z80Chip>(std::move(params), memory, std::move(ports));
  }
}  // namespace Devices::Z80
