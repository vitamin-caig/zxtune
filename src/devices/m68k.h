/**
 *
 * @file
 *
 * @brief  Motorola 68000 machine core interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/view.h"
#include "time/instant.h"
#include "time/oscillator.h"

#include "types.h"

#include <memory>

// Generic 68k CPU + RAM + peripheral bus; machine-specific parts (chips, exception vectors,
// playback model) are built on top by the owners.
namespace Devices::M68K
{
  using TimePoint = Time::AtMicrosecond;
  using Oscillator = Time::MicrosecOscillator;

  //! Peripheral attached to the CPU bus at a fixed address window. The core routes only addresses
  //! within the registered window here, so a device decodes its own sub-registers.
  class Device
  {
  public:
    virtual ~Device() = default;

    virtual uint8_t ReadByte(uint32_t addr) = 0;
    virtual uint16_t ReadWord(uint32_t addr) = 0;
    //! Writes carry the master clock so time-sensitive peripherals can stamp the change
    virtual void WriteByte(uint32_t addr, uint8_t data, const Oscillator& clock) = 0;
    virtual void WriteWord(uint32_t addr, uint16_t data, const Oscillator& clock) = 0;
  };

  //! Notified after every executed CPU slice so the owner can keep peripherals in step with the clock
  class CycleObserver
  {
  public:
    virtual ~CycleObserver() = default;

    virtual void OnCycles(uint32_t cycles) = 0;
  };

  //! Generic 68000 machine: the CPU, its RAM, a peripheral bus and the master clock.
  class Chip
  {
  public:
    using Ptr = std::unique_ptr<Chip>;
    virtual ~Chip() = default;

    //! Clear the RAM, reset the CPU/clock and plant the subroutine return sentinel
    virtual void Reset() = 0;

    //! Store raw big-endian data at the given RAM address
    virtual void Load(uint32_t addr, Binary::View data) = 0;

    //! Attach a peripheral serving the half-open address window [lo, hi)
    virtual void AddDevice(uint32_t lo, uint32_t hi, Device& device) = 0;

    virtual uint8_t ReadRamByte(uint32_t addr) const = 0;
    virtual uint16_t ReadRamWord(uint32_t addr) const = 0;
    virtual void WriteRamWord(uint32_t addr, uint16_t data) = 0;
    virtual void WriteDword(uint32_t addr, uint32_t data) = 0;
    virtual void WriteBlock(uint32_t addr, Binary::View data) = 0;

    //! Current master time, shared by the bus and by externally clocked devices
    virtual TimePoint GetTime() const = 0;
    //! Move the master time forward (never backwards)
    virtual void SetTime(TimePoint time) = 0;

    //! Call a subroutine and run until it returns. The observer (may be null) is notified with the
    //! cycles consumed by every slice, and the master time is advanced accordingly. Returns false
    //! if the driver faulted (runaway execution).
    virtual bool ExecuteSubroutine(uint32_t addr, uint32_t d0reg, uint32_t cycleBudget, CycleObserver* observer) = 0;

    //! Invoke the interrupt handler installed at the given vector and run until it returns
    virtual bool ExecuteInterrupt(uint32_t vector, uint32_t cycleBudget, CycleObserver* observer) = 0;

    virtual bool Faulted() const = 0;
    virtual void SetFaulted(bool faulted) = 0;
  };

  //! Create a 68000 core with the given CPU clock (Hz) and RAM size (bytes)
  Chip::Ptr CreateChip(uint_t clockFreq, uint_t ramSize);
}  // namespace Devices::M68K
