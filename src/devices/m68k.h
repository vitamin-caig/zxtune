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
  using Oscillator = Time::MicrosecOscillator;

  //! Peripheral attached to the CPU bus at a fixed address window. The core routes only addresses
  //! within the registered window here, so a device decodes its own sub-registers.
  class Device
  {
  public:
    virtual ~Device() = default;

    virtual uint8_t ReadByte(uint32_t addr) = 0;
    virtual uint16_t ReadWord(uint32_t addr) = 0;
    //! Writes carry the master clock positioned at the bus access cycle, so time-sensitive
    //! peripherals can stamp the change exactly
    virtual void WriteByte(uint32_t addr, uint8_t data, const Oscillator& clock) = 0;
    virtual void WriteWord(uint32_t addr, uint16_t data, const Oscillator& clock) = 0;
  };

  //! Execution environment of the chip. Every executed slice is reported with the master
  //! oscillator at the slice end, so the owner can keep the peripherals in step without reaching
  //! back into the chip, and the pending interrupt sources are resolved at the acknowledge cycle.
  class Environment
  {
  public:
    virtual ~Environment() = default;

    //! Called after every executed CPU slice with the master oscillator at the slice end
    virtual void OnCycles(const Oscillator& clock) = 0;

    //! Vector number (table address is the vector times four) to serve the acknowledged level;
    //! NO_VECTOR lets the CPU use its autovector
    virtual uint32_t GetInterruptVector(uint_t level) = 0;
  };

  //! Returned when the level has no externally supplied vector
  constexpr uint32_t NO_VECTOR = 0xffffffff;

  //! External clock parameters source; polled by the chip so clock changes apply at runtime
  class ChipParameters
  {
  public:
    using Ptr = std::shared_ptr<const ChipParameters>;
    virtual ~ChipParameters() = default;

    virtual uint_t Version() const = 0;
    virtual uint_t ClockFreq() const = 0;
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

    //! Master oscillator: GetCurrentTick() is the cumulative CPU cycle odometer, GetCurrentTime()
    //! is the externally visible timeline. Bus writes are stamped with the odometer position of
    //! their access; between bursts it is advanced to the exact number of executed (or idled)
    //! cycles.
    virtual const Oscillator& GetOscillator() const = 0;
    //! Move the master timeline forward (never backwards)
    virtual void AdvanceTime(Time::AtMicrosecond time) = 0;

    //! Call a subroutine and run until it returns. Executed cycles always advance the master time
    //! and are reported to the environment. Returns false if the driver faulted (runaway execution).
    virtual bool ExecuteSubroutine(uint32_t addr, uint32_t d0reg, uint32_t cycleBudget) = 0;

    //! Set the CPU interrupt priority level (0 releases the line); the CPU samples it at the slice
    //! boundaries and preempts the running code according to its current priority mask
    virtual void SetIrqLevel(uint_t level) = 0;

    //! Execute the CPU up to the given time. The execution starts or resumes wherever the previous
    //! run stopped, so a taken interrupt preempts the interrupted code and returns to it on RTE.
    //! Executed cycles always advance the master time and are reported to the environment.
    virtual void RunUntil(Time::AtMicrosecond time) = 0;

    virtual bool Faulted() const = 0;
    virtual void SetFaulted(bool faulted) = 0;
  };

  //! Create a 68000 core with the given execution environment and RAM size (bytes); the CPU clock is
  //! polled from the parameters
  Chip::Ptr CreateChip(Environment& env, ChipParameters::Ptr params, uint_t ramSize);
}  // namespace Devices::M68K
