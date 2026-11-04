/**
 *
 * @file
 *
 * @brief  Motorola 68000 machine core implementation
 *
 * @author vitamin.caig@gmail.com
 *
 * @note The CPU backend is the 3rdparty c68k interpreter (3rdparty/ht/Core/c68k).
 *
 **/

#include "devices/m68k.h"

#include "debug/log.h"
#include "parameters/tracking_helper.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"

#include "3rdparty/ht/Core/c68k/c68k.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace Devices::M68K
{
  // The 68000 has a 24-bit address bus
  const uint32_t ADDR_MASK = 0xffffff;
  // Subroutines return into the first word of the guard bank right after the RAM (see below): it is
  // always a branch-to-self and drivers cannot reach it, unlike the low system RAM area they use as
  // scratch. The return checker only looks at the stack pointer, so the address itself is free.
  const uint16_t BRA_SELF = 0x60fe;
  // Size of the branch-to-self bank mapped right after the RAM. The core fetches instructions
  // linearly through a raw pointer, so execution that runs off the end of the RAM must land in a
  // mapped bank; a self-branch traps the runaway in place until the cycle budget gives up.
  const uint32_t GUARD_SIZE = 1 << 16;
  // Microseconds between the observer notifications during free-running execution
  const uint32_t OBSERVER_INTERVAL = 8;

  const Debug::Stream Dbg("Devices::M68K");

  class ChipImpl : public Chip
  {
  public:
    ChipImpl(Environment& env, ChipParameters::Ptr params, uint_t ramSize)
      : Env(env)
      , Params(std::move(params))
      , RamSize(ramSize)
      , Ram((ramSize + GUARD_SIZE) / sizeof(uint16_t))
    {
      // c68k fetches directly from mapped banks; the bank right after the RAM is a branch-to-self
      // one so that linear execution running off the RAM end cannot walk into unmapped host memory.
      // The remaining non-RAM banks point at an identical branch-to-self page.
      Dummy.resize(1 << 15);
      std::fill(Dummy.begin(), Dummy.end(), BRA_SELF);
      ::C68k_Init(&Cpu, &InterruptAckTrampoline);
      ::C68k_Set_Callback_Param(&Cpu, this);
      ::C68k_Set_ReadB(&Cpu, &ReadByteTrampoline);
      ::C68k_Set_ReadW(&Cpu, &ReadWordTrampoline);
      ::C68k_Set_WriteB(&Cpu, &WriteByteTrampoline);
      ::C68k_Set_WriteW(&Cpu, &WriteWordTrampoline);
      ::C68k_Set_Fetch(&Cpu, 0, RamSize + GUARD_SIZE - 1, reinterpret_cast<pointer>(Ram.data()));
      for (uint32_t base = RamSize + GUARD_SIZE; base != 0x1000000; base += 1 << 16)
      {
        ::C68k_Set_Fetch(&Cpu, base, base + 0xffff, reinterpret_cast<pointer>(Dummy.data()));
      }
      Reset();
    }

    void Reset() override
    {
      std::fill(Ram.begin(), Ram.end(), uint16_t(0));
      std::fill(Ram.begin() + RamSize / sizeof(uint16_t), Ram.end(), BRA_SELF);
      // Oscillator::Reset drops the frequency along with the phase, so restore it
      Params.Reset();
      Clock.Reset();
      SynchronizeParameters();
      FaultedState = false;
      SetSR(0x2700);
      SetAReg(7, RamSize);
      SetPC(0);
    }

    void Load(uint32_t addr, Binary::View data) override
    {
      const auto size = data.Size();
      Require(addr + size <= RamSize);
      // Store the big-endian driver words in the native order expected by c68k
      const auto words = size / sizeof(uint16_t);
      if (words)
      {
        auto* const dst = Ram.data() + addr / sizeof(uint16_t);
        std::copy_n(data.As<be_uint16_t>(), words, dst);
      }
      if (size & 1)
      {
        SetByte(addr + size - 1, data.As<uint8_t>()[size - 1]);
      }
    }

    void AddDevice(uint32_t lo, uint32_t hi, Device& device) override
    {
      Require(lo < hi);
      const auto pos = std::lower_bound(Bus.begin(), Bus.end(), lo,
                                        [](const BusDevice& entry, uint32_t value) { return entry.Lo < value; });
      Bus.insert(pos, BusDevice{lo, hi, &device});
    }

    uint8_t ReadRamByte(uint32_t addr) const override
    {
      const auto a = addr & ADDR_MASK;
      return a < RamSize ? GetByte(a) : 0;
    }

    uint16_t ReadRamWord(uint32_t addr) const override
    {
      const auto a = addr & ADDR_MASK;
      return a + 1 < RamSize ? ReadWordRaw(a) : 0;
    }

    void WriteRamWord(uint32_t addr, uint16_t data) override
    {
      const auto a = addr & ADDR_MASK;
      if (a + 1 < RamSize)
      {
        WriteWordRaw(a, data);
      }
    }

    void WriteDword(uint32_t addr, uint32_t data) override
    {
      WriteWordRaw(addr, uint16_t(data >> 16));
      WriteWordRaw(addr + sizeof(uint16_t), uint16_t(data));
    }

    void WriteBlock(uint32_t addr, Binary::View data) override
    {
      const auto* const bytes = data.As<uint8_t>();
      for (std::size_t idx = 0; idx != data.Size(); ++idx)
      {
        SetByte(addr + idx, bytes[idx]);
      }
    }

    const Oscillator& GetOscillator() const override
    {
      return Clock;
    }

    void AdvanceTime(Time::AtMicrosecond time) override
    {
      Clock.AdvanceTime(time);
    }

    void SetIrqLevel(uint_t level) override
    {
      // C68k_Set_IRQ also clears the halted state, so a released line must not be re-poked: an idle
      // STOP has to stay halted until a new interrupt actually arrives
      if (0 == level && 0 == IrqLevel)
      {
        return;
      }
      IrqLevel = level;
      ::C68k_Set_IRQ(&Cpu, int(level));
    }

    void RunUntil(Time::AtMicrosecond time) override
    {
      // Runtime clock changes take effect at the slice boundary
      SynchronizeParameters();
      uint32_t sinceObserver = ObserverQuantum;
      while (Clock.GetCurrentTime() < time)
      {
        // One microsecond slices keep the interrupt sampling and the bus access stamps on the
        // output resolution while a driver runs its main loop or a handler
        const auto cycles = ::C68k_Exec(&Cpu, int(ObservedQuantum));
        if (cycles > 0)
        {
          // Every executed cycle is accounted at the master odometer; the observer additionally
          // keeps the peripherals (e.g. the DMA stream) in step with the executed code. The
          // notification is decimated to keep a long free-running main loop affordable; the
          // eventual precision is still far below one output sample.
          Clock.AdvanceTick(uint64_t(cycles));
          if (sinceObserver >= ObserverQuantum)
          {
            sinceObserver = 0;
            Env.OnCycles(Clock);
          }
          sinceObserver += uint32_t(cycles);
        }
        else
        {
          // A halted (STOP) or faulted CPU consumes the timeline without executing instructions
          FaultedState = FaultedState || cycles < 0;
          Clock.AdvanceTime(time);
          break;
        }
      }
    }

    bool ExecuteSubroutine(uint32_t addr, uint32_t d0reg, uint32_t cycleBudget) override
    {
      const uint32_t sp = RamSize;
      // Return address for the trailing RTS points at the loop sentinel
      WriteDword(sp - sizeof(uint32_t), sp);
      SetDReg(0, d0reg);
      SetAReg(7, sp - sizeof(uint32_t));
      SetPC(addr);
      SetSR(0x2300);
      const bool wasFaulted = FaultedState;
      Run(sp, cycleBudget);
      SetAReg(7, sp);
      // Drivers are plain subroutines and may leave the interrupt mask raised; like RTE of the
      // caller would, restore the caller context so the playback stays interruptible
      SetSR(0x2300);
      if (!wasFaulted && FaultedState)
      {
        Dbg("Subroutine ${:06x} timed out at pc=${:06x}", addr, GetPC());
      }
      return !FaultedState;
    }

    bool Faulted() const override
    {
      return FaultedState;
    }

    void SetFaulted(bool faulted) override
    {
      FaultedState = faulted;
    }

  private:
    // CPU register helpers backing the run-to-marker call machinery
    uint32_t GetPC() const
    {
      return ::C68k_Get_PC(const_cast<c68k_struc*>(&Cpu));
    }

    void SetDReg(uint_t idx, uint32_t value)
    {
      ::C68k_Set_DReg(&Cpu, idx, value);
    }

    uint32_t GetAReg(uint_t idx) const
    {
      return ::C68k_Get_AReg(const_cast<c68k_struc*>(&Cpu), idx);
    }

    void SetAReg(uint_t idx, uint32_t value)
    {
      ::C68k_Set_AReg(&Cpu, idx, value);
    }

    void SetPC(uint32_t value)
    {
      ::C68k_Set_PC(&Cpu, value);
    }

    void SetSR(uint32_t value)
    {
      ::C68k_Set_SR(&Cpu, value);
    }

    struct BusDevice
    {
      uint32_t Lo;
      uint32_t Hi;
      Device* Unit;
    };

    //! Stamp for the bus access currently being executed: the burst-start odometer advanced by the
    //! cycles already spent in the burst. The main clock itself only moves at burst boundaries.
    Oscillator GetAccessTime()
    {
      // The CPU timeline is always monotonic; the burst start plus the cycles done is the exact
      // access time, independently of whether the run is observed
      const auto done = ::C68k_Get_CycleDone(&Cpu);
      Oscillator stamp = Clock;
      if (done > 0)
      {
        stamp.AdvanceTick(uint64_t(done));
      }
      return stamp;
    }

    //! Run until the stack pointer reaches the limit, the budget is spent or the driver faults.
    //! Executed cycles advance the master clock odometer; the environment is notified so the owner
    //! can keep the peripherals (e.g. timers) in step.
    void Run(uint32_t sp, uint32_t budget)
    {
      // Runtime clock changes take effect at the slice boundary
      SynchronizeParameters();
      uint32_t sinceObserver = ObserverQuantum;
      uint32_t done = 0;
      while (GetAReg(7) < sp)
      {
        const auto cycles = ::C68k_Exec(&Cpu, int(ObservedQuantum));
        if (cycles > 0)
        {
          Clock.AdvanceTick(uint64_t(cycles));
          if (sinceObserver >= ObserverQuantum)
          {
            sinceObserver = 0;
            Env.OnCycles(Clock);
          }
          sinceObserver += uint32_t(cycles);
        }
        if (cycles < 0 || (done += uint32_t(cycles)) > budget)
        {
          FaultedState = true;
          break;
        }
      }
    }

    Device* FindDevice(uint32_t addr) const
    {
      const auto pos = std::upper_bound(Bus.begin(), Bus.end(), addr,
                                        [](uint32_t value, const BusDevice& entry) { return value < entry.Lo; });
      if (pos != Bus.begin())
      {
        const auto& prev = *(pos - 1);
        if (addr < prev.Hi)
        {
          return prev.Unit;
        }
      }
      return nullptr;
    }

    uint8_t ReadByte(uint32_t addr) const
    {
      const auto a = addr & ADDR_MASK;
      if (a < RamSize)
      {
        return GetByte(a);
      }
      if (auto* const device = FindDevice(a))
      {
        return device->ReadByte(a);
      }
      return 0xff;
    }

    uint16_t ReadWord(uint32_t addr) const
    {
      const auto a = addr & ADDR_MASK;
      if (a + 1 < RamSize)
      {
        return ReadWordRaw(a);
      }
      if (auto* const device = FindDevice(a))
      {
        return device->ReadWord(a);
      }
      return 0xffff;
    }

    void WriteByte(uint32_t addr, uint8_t data)
    {
      const auto a = addr & ADDR_MASK;
      if (a < RamSize)
      {
        SetByte(a, data);
      }
      else if (auto* const device = FindDevice(a))
      {
        device->WriteByte(a, data, GetAccessTime());
      }
    }

    void WriteWord(uint32_t addr, uint16_t data)
    {
      const auto a = addr & ADDR_MASK;
      if (a + 1 < RamSize)
      {
        WriteWordRaw(a, data);
      }
      else if (auto* const device = FindDevice(a))
      {
        device->WriteWord(a, data, GetAccessTime());
      }
    }

    // Bus accessors operating on the RAM part only. RAM keeps native 68k words, so the byte at an
    // even address is the high half of the word, and the one at an odd address is the low half.
    uint8_t GetByte(uint32_t addr) const
    {
      const auto word = Ram[addr / sizeof(uint16_t)];
      return (addr & 1) ? uint8_t(word) : uint8_t(word >> 8);
    }

    void SetByte(uint32_t addr, uint8_t data)
    {
      auto& word = Ram[addr / sizeof(uint16_t)];
      if (addr & 1)
      {
        word = uint16_t((word & 0xff00) | data);
      }
      else
      {
        word = uint16_t((word & 0x00ff) | (uint16_t(data) << 8));
      }
    }

    uint16_t ReadWordRaw(uint32_t addr) const
    {
      if (addr & 1)
      {
        return (uint16_t(GetByte(addr)) << 8) | GetByte(addr + 1);
      }
      return Ram[addr / sizeof(uint16_t)];
    }

    void WriteWordRaw(uint32_t addr, uint16_t data)
    {
      if (addr & 1)
      {
        SetByte(addr, uint8_t(data >> 8));
        SetByte(addr + 1, uint8_t(data));
      }
      else
      {
        Ram[addr / sizeof(uint16_t)] = data;
      }
    }

    static u32 FASTCALL ReadByteTrampoline(void* param, const u32 addr)
    {
      return static_cast<const ChipImpl*>(param)->ReadByte(addr);
    }

    static u32 FASTCALL ReadWordTrampoline(void* param, const u32 addr)
    {
      return static_cast<const ChipImpl*>(param)->ReadWord(addr);
    }

    static void FASTCALL WriteByteTrampoline(void* param, const u32 addr, u32 data)
    {
      static_cast<ChipImpl*>(param)->WriteByte(addr, uint8_t(data));
    }

    static void FASTCALL WriteWordTrampoline(void* param, const u32 addr, u32 data)
    {
      static_cast<ChipImpl*>(param)->WriteWord(addr, uint16_t(data));
    }

    static s32 FASTCALL InterruptAckTrampoline(void* param, s32 level)
    {
      const auto* const self = static_cast<const ChipImpl*>(param);
      const auto vector = self->Env.GetInterruptVector(uint_t(level));
      return vector == NO_VECTOR ? s32(C68K_INT_ACK_AUTOVECTOR) : s32(vector);
    }

  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        ClockFreq = Params->ClockFreq();
        Clock.SetFrequency(ClockFreq);
        ObservedQuantum = std::max<uint32_t>(uint_t(ClockFreq / Time::Microsecond::PER_SECOND), 1);
        ObserverQuantum = std::max<uint32_t>(uint_t(ClockFreq * OBSERVER_INTERVAL / Time::Microsecond::PER_SECOND), 1);
      }
    }

  private:
    Environment& Env;
    Parameters::TrackingHelper<ChipParameters> Params;
    const uint32_t RamSize;
    c68k_struc Cpu;
    std::vector<uint16_t> Ram;
    std::vector<uint16_t> Dummy;
    // Devices sorted by their window start
    std::vector<BusDevice> Bus;
    // CPU cycle odometer the master time and the device IO stamps derive from
    Time::MicrosecOscillator Clock;
    uint_t ClockFreq = 0;
    // One microsecond worth of CPU cycles is the natural resolution of the observed timeline
    uint32_t ObservedQuantum = 1;
    // Cycles between the observer notifications (see OBSERVER_INTERVAL)
    uint32_t ObserverQuantum = 1;
    // Currently asserted interrupt line, kept to avoid re-poking C68k (it clears the halted state)
    uint_t IrqLevel = 0;
    bool FaultedState = false;
  };

  Chip::Ptr CreateChip(Environment& env, ChipParameters::Ptr params, uint_t ramSize)
  {
    return MakePtr<ChipImpl>(env, std::move(params), ramSize);
  }
}  // namespace Devices::M68K
