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
  // Subroutines are called by planting this address as their return; it points to an endless loop so
  // that the CPU keeps running harmlessly while the stack pointer is checked
  const uint32_t RETURN_SENTINEL = 0x4fc;
  const uint16_t BRA_SELF = 0x60fe;
  // Number of CPU cycles executed between fault/budget checks
  const uint32_t EXEC_SLICE = 64;

  const Debug::Stream Dbg("Devices::M68K");

  class ChipImpl : public Chip
  {
  public:
    ChipImpl(uint_t clockFreq, uint_t ramSize)
      : ClockFreq(clockFreq)
      , RamSize(ramSize)
      , Ram(ramSize / sizeof(uint16_t))
    {
      // c68k fetches directly from mapped banks; point all non-RAM banks at a branch-to-self page.
      // NOPs would let linear execution walk off this single page into unmapped host memory; the
      // self-branch traps a runaway in place until the cycle budget gives up.
      Dummy.resize(1 << 15);
      std::fill(Dummy.begin(), Dummy.end(), BRA_SELF);
      ::C68k_Init(&Cpu, nullptr);
      ::C68k_Set_Callback_Param(&Cpu, this);
      ::C68k_Set_ReadB(&Cpu, &ReadByteTrampoline);
      ::C68k_Set_ReadW(&Cpu, &ReadWordTrampoline);
      ::C68k_Set_WriteB(&Cpu, &WriteByteTrampoline);
      ::C68k_Set_WriteW(&Cpu, &WriteWordTrampoline);
      ::C68k_Set_Fetch(&Cpu, 0, RamSize - 1, reinterpret_cast<pointer>(Ram.data()));
      for (uint32_t base = RamSize; base != 0x1000000; base += 1 << 16)
      {
        ::C68k_Set_Fetch(&Cpu, base, base + 0xffff, reinterpret_cast<pointer>(Dummy.data()));
      }
      Reset();
    }

    void Reset() override
    {
      std::fill(Ram.begin(), Ram.end(), uint16_t(0));
      // Oscillator::Reset drops the frequency along with the phase, so restore it
      Clock.Reset();
      Clock.SetFrequency(ClockFreq);
      FaultedState = false;
      // Return sentinel for the run-to-marker subroutine call technique
      WriteWordRaw(RETURN_SENTINEL, BRA_SELF);
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

    TimePoint GetTime() const override
    {
      return Clock.GetCurrentTime();
    }

    void SetTime(TimePoint time) override
    {
      // GetTickAtTime wraps when the target is not strictly ahead, so compare the integer time first
      if (Clock.GetCurrentTime() < time)
      {
        Clock.AdvanceTick(Clock.GetTickAtTime(time) - Clock.GetCurrentTick());
      }
    }

    bool ExecuteSubroutine(uint32_t addr, uint32_t d0reg, uint32_t cycleBudget, CycleObserver* observer) override
    {
      const uint32_t sp = RamSize;
      // Return address for the trailing RTS points at the loop sentinel
      WriteDword(sp - sizeof(uint32_t), RETURN_SENTINEL);
      SetDReg(0, d0reg);
      SetAReg(7, sp - sizeof(uint32_t));
      SetPC(addr);
      SetSR(0x2300);
      const bool wasFaulted = FaultedState;
      Run(sp, cycleBudget, observer);
      SetAReg(7, sp);
      if (!wasFaulted && FaultedState)
      {
        Dbg("Subroutine ${:06x} timed out at pc=${:06x}", addr, GetPC());
      }
      return !FaultedState;
    }

    bool ExecuteInterrupt(uint32_t vector, uint32_t cycleBudget, CycleObserver* observer) override
    {
      const auto handler = ReadDword(vector);
      if (0 == handler)
      {
        return !FaultedState;
      }
      const uint32_t sp = RamSize;
      // RTE frame: [SR][PC]; the returned PC points at the loop sentinel
      WriteWordRaw(sp - 6, 0x2300);
      WriteDword(sp - sizeof(uint32_t), RETURN_SENTINEL);
      SetAReg(7, sp - 6);
      SetPC(handler);
      SetSR(0x2300);
      const bool wasFaulted = FaultedState;
      Run(sp, cycleBudget, observer);
      SetAReg(7, sp);
      if (!wasFaulted && FaultedState)
      {
        Dbg("Interrupt ${:06x} timed out at pc=${:06x}", vector, GetPC());
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
    // CPU register and RAM helpers backing the run-to-marker call/interrupt machinery
    uint32_t ReadDword(uint32_t addr) const
    {
      return (uint32_t(ReadWordRaw(addr)) << 16) | ReadWordRaw(addr + sizeof(uint16_t));
    }

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

    //! Run until the stack pointer reaches the limit, the budget is spent or the driver faults.
    //! When an observer is given, the executed cycles advance the master time and are reported so the
    //! owner can keep peripherals (e.g. timers) in step.
    void Run(uint32_t sp, uint32_t budget, CycleObserver* observer)
    {
      uint32_t done = 0;
      while (GetAReg(7) < sp)
      {
        const auto cycles = ::C68k_Exec(&Cpu, EXEC_SLICE);
        if (observer && cycles > 0)
        {
          // Executed cycles consume the output timeline; the IO inside the slice keeps its start stamp
          Clock.AdvanceTick(uint64_t(cycles));
          observer->OnCycles(uint32_t(cycles));
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
        device->WriteByte(a, data, Clock);
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
        device->WriteWord(a, data, Clock);
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

  private:
    const uint_t ClockFreq;
    const uint32_t RamSize;
    c68k_struc Cpu;
    std::vector<uint16_t> Ram;
    std::vector<uint16_t> Dummy;
    // Devices sorted by their window start
    std::vector<BusDevice> Bus;
    // Master timeline; the CPU cycles and the device IO stamps derive from it
    Time::MicrosecOscillator Clock;
    bool FaultedState = false;
  };

  Chip::Ptr CreateChip(uint_t clockFreq, uint_t ramSize)
  {
    return MakePtr<ChipImpl>(clockFreq, ramSize);
  }
}  // namespace Devices::M68K
