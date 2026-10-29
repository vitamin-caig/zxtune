/**
 *
 * @file
 *
 * @brief  SNDH chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 * @note The Atari ST machine emulation (the MFP event mode of src/Mk68901.cpp and the wiring of
 * src/AtariMachine.cpp) is based on sources of AtariAudio (sndh-player) by Arnaud Carré aka
 * Leonard/Oxygene (@leonard_coder). The STe DMA sound device is implemented by the SNDH renderer.
 *
 **/

#include "module/players/aym/sndh.h"

#include "devices/aym/chip.h"
#include "devices/details/clock_source.h"
#include "module/players/aym/ay_data_channel.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/duration.h"
#include "module/players/platforms.h"
#include "module/players/properties_meta.h"
#include "module/players/streaming.h"

#include "binary/dump.h"
#include "binary/view.h"
#include "debug/log.h"
#include "devices/m68k.h"
#include "formats/multitrack/sndh.h"
#include "sound/gain.h"
#include "time/oscillator.h"

#include "contract.h"
#include "make_ptr.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>

namespace Module::SNDH
{
  const Debug::Stream Dbg("Core::SNDHSupp");

  //! SNDH modules are played on an Atari ST with its stock 2 MHz YM2149F
  const uint_t YM_CLOCK = 2000000;
  const uint_t DEFAULT_FREQUENCY = 50;

  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;

    // FRMS/tickrate is usually not an integer number of seconds, so keep the full resolution
    Time::Microseconds Duration;
    uint_t Frequency = DEFAULT_FREQUENCY;
    //! 1-based subsong index
    uint_t Index = 1;
    Binary::Dump RAM;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100).CastTo<Time::Microsecond>();

  // Atari ST machine: the 68k core plus the MFP/shifter stubs and the SNDH playback model
  const uint32_t RAM_SIZE = 4 * 1024 * 1024;
  // Standard SNDH upload address (2 bytes past a 64k boundary)
  const uint32_t UPLOAD_ADDR = 0x10002;

  // Safety limits for burst CPU execution; generously sized because drivers may preprocess sample
  // data (e.g. the STe DMA ones) before returning from init
  const uint32_t INIT_CYCLES = 1 << 26;
  const uint32_t CALL_CYCLES = 1 << 22;

  // Front stub occupies the historical TRAP #14 vector address so drivers that read the vector
  // see an unchanged value; the api68 blob sits just behind it
  const uint32_t TRAP14_FRONT_ADDR = 0x600;
  const uint32_t TRAP14_ADDR = 0x620;

  // Vectors of the MFP timer handlers (A, B, C, D, GPI7)
  const uint32_t TIMER_VECTORS[] = {0x134, 0x120, 0x114, 0x110, 0x13c};

  // Handler installed as the MFP timer C interrupt: calls the driver play routine at UPLOAD_ADDR+8
  // and returns from the interrupt
  const uint32_t PLAY_STUB_ADDR = 0x480;
  const uint8_t PLAY_STUB[] = {
      0x42, 0x80,                          // clr.l d0
      0x4e, 0xb9, 0x00, 0x01, 0x00, 0x0a,  // jsr $0001000a
      0x4e, 0x73,                          // rte
  };

  // from api68.c; implements XBIOS 31 (Xbtimer), 32 (DoSound) and 38 (Supexec)
  const uint8_t TRAP14[] = {
      0x0c, 0x6f, 0x00, 0x26, 0x00, 0x06, 0x67, 0x00, 0x00, 0x22, 0x0c, 0x6f, 0x00, 0x1f, 0x00, 0x06, 0x67, 0x00, 0x00,
      0x28, 0x0c, 0x6f, 0x00, 0x22, 0x00, 0x06, 0x67, 0x00, 0x00, 0x86, 0x0c, 0x6f, 0x00, 0x0e, 0x00, 0x06, 0x67, 0x00,
      0x00, 0xec, 0x4e, 0x73, 0x48, 0xe7, 0xff, 0xfe, 0x20, 0x6f, 0x00, 0x44, 0x4e, 0x90, 0x4c, 0xdf, 0x7f, 0xff, 0x4e,
      0x73, 0x48, 0xe7, 0xff, 0xfe, 0x41, 0xef, 0x00, 0x44, 0x4c, 0x98, 0x00, 0x07, 0x02, 0x40, 0x00, 0x03, 0xd0, 0x40,
      0x16, 0x38, 0xfa, 0x17, 0x02, 0x43, 0x00, 0xf0, 0xe5, 0x4b, 0x36, 0x7b, 0x00, 0x42, 0xd6, 0xc3, 0x76, 0x00, 0x45,
      0xf8, 0xfa, 0x1f, 0xd4, 0xc0, 0x43, 0xf8, 0xfa, 0x19, 0xd2, 0xc0, 0xe2, 0x48, 0x04, 0x00, 0x00, 0x02, 0x6b, 0x1a,
      0x57, 0xc3, 0x18, 0x03, 0x0a, 0x03, 0x00, 0xf0, 0x44, 0x04, 0x48, 0x84, 0xd8, 0x44, 0x43, 0xf1, 0x40, 0xfe, 0xd8,
      0x44, 0x02, 0x41, 0x00, 0x0f, 0xe9, 0x69, 0xc7, 0x11, 0x14, 0x82, 0x26, 0x90, 0x83, 0x11, 0x4c, 0xdf, 0x7f, 0xff,
      0x4e, 0x73, 0x00, 0x34, 0x00, 0x20, 0x00, 0x14, 0x00, 0x10, 0x48, 0xe7, 0x00, 0xc0, 0x43, 0xfa, 0x00, 0x20, 0x70,
      0x00, 0x20, 0x7b, 0x00, 0x3e, 0x41, 0xfb, 0x80, 0x5e, 0x23, 0x88, 0x00, 0x00, 0x58, 0x40, 0xb0, 0x7c, 0x00, 0x24,
      0x66, 0xec, 0x20, 0x09, 0x4c, 0xdf, 0x03, 0x00, 0x4e, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4e, 0x75, 0xc1, 0x88, 0x41,
      0xfa, 0x00, 0x0c, 0x21, 0x48, 0x00, 0x04, 0x58, 0x48, 0xc1, 0x88, 0x4e, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
  };

  // GEMDOS (TRAP #1) stub. Drivers are normal Atari programs, so some (e.g. Maxymizer) request
  // their sample/work buffers via Malloc at init. Emulate an incremental allocator; Sversion and
  // the unimplemented file/console calls answer 0.
  // Placed below the GEMDOS heap rather than in page 0: many drivers use the low system area
  // around 0x400-0x800 as scratch, so a stub there would clobber their data
  const uint32_t GEMDOS_ADDR = 0x2ff000;
  // Malloc's next-free pointer lives right after the stub; Reset initialises it
  const uint32_t GEMDOS_NEXT_ADDR = 0x2ff100;
  const uint32_t GEMDOS_MALLOC_START = 0x300000;
  const uint8_t GEMDOS[] = {
      0x30, 0x2f, 0x00, 0x06,                          // move.w 6(a7),d0 (function)
      0x0c, 0x40, 0x00, 0x48, 0x67, 0x0e,              // cmpi.w #$48,d0 ; beq.b malloc
      0x0c, 0x40, 0x00, 0x30, 0x67, 0x04,              // cmpi.w #$30,d0 ; beq.b sversion
      0x42, 0x80, 0x4e, 0x73,                          // clr.l d0 ; rte (Mfree/Mshrink/unknown)
      0x42, 0x80, 0x4e, 0x73,                          // sversion: clr.l d0 ; rte
      0x22, 0x2f, 0x00, 0x08,                          // malloc: move.l 8(a7),d1 (size)
      0x20, 0x39, 0x00, 0x2f, 0xf1, 0x00,              // move.l GEMDOS_NEXT_ADDR,d0
      0x0c, 0x81, 0xff, 0xff, 0xff, 0xff, 0x67, 0x1c,  // cmpi.l #-1,d1 ; beq.b largest
      0x24, 0x00, 0xd4, 0x81, 0x52, 0x82,              // move.l d0,d2 ; add.l d1,d2 ; addq.l #1,d2
      0x02, 0x82, 0xff, 0xff, 0xff, 0xfe,              // andi.l #$fffffffe,d2
      0x0c, 0x82, 0x00, 0x40, 0x00, 0x00, 0x64, 0x14,  // cmpi.l #RAM_SIZE,d2 ; bcc.b fail
      0x23, 0xc2, 0x00, 0x2f, 0xf1, 0x00,              // move.l d2,GEMDOS_NEXT_ADDR
      0x4e, 0x73,                                      // rte
      0x24, 0x3c, 0x00, 0x40, 0x00, 0x00,              // largest: move.l #RAM_SIZE,d2
      0x94, 0x80, 0x20, 0x02, 0x4e, 0x73,              // sub.l d0,d2 ; move.l d2,d0 ; rte
      0x42, 0x80, 0x4e, 0x73,                          // fail: clr.l d0 ; rte
  };

  // The api68 TRAP14 blob does not answer XBIOS 64 (Blitmode), so a tiny front stub answers it
  // (blitter present + enabled) and otherwise jumps to the api68 code; drivers that probe for the
  // blitter before using it need this
  const uint8_t TRAP14_FRONT[] = {
      0x0c, 0x6f, 0x00, 0x40, 0x00, 0x06,  // cmpi.w #$40,6(a7) (Blitmode)
      0x66, 0x04,                          // bne.b api68
      0x70, 0x03,                          // moveq #3,d0 (present + enabled)
      0x4e, 0x73,                          // rte
      0x4e, 0xf9, 0x00, 0x00, 0x06, 0x20,  // jmp TRAP14_ADDR
  };

  // Cookie jar (_SND sound chip, _MCH machine) laid out as big-endian dwords; Maxymizer and
  // similar drivers scan it to detect the sound hardware
  const uint32_t COOKIE_JAR_ADDR = 0x900;
  const uint8_t COOKIE_JAR[] = {
      0x5f, 0x53, 0x4e, 0x44,  // '_SND'
      0x00, 0x00, 0x00, 0x03,  // sound chip + STE DMA
      0x5f, 0x4d, 0x43, 0x48,  // '_MCH'
      0x00, 0x01, 0x00, 0x00,  // STE
      0x00, 0x00, 0x00, 0x00,  // end
  };

  // Atari ST CPU/system clock; both the output timeline and the IO stamps derive from it
  const uint32_t CPU_CLOCK = 8000000;

  //! MFP 68901 timer block. Timers are clocked by the output timeline (one host sample at a time
  //! in the reference implementation), not by CPU cycles, because the CPU only runs in bursts.
  class Mfp : public Devices::M68K::Device
  {
  public:
    //! Register window within the 24-bit address space
    static const uint32_t WINDOW_LO = 0xfffa00;
    static const uint32_t WINDOW_HI = WINDOW_LO + 0x26;
    //! The MFP is clocked directly from the system clock
    static constexpr uint32_t CLOCK = 2457600;
    //! Divisors applied to the MFP clock for timer control register values 0x0..0x7
    static constexpr uint32_t PRESCALE[] = {0, 4, 10, 16, 50, 64, 100, 200};

    //! Registers (byte offsets within WINDOW_LO)
    enum Register
    {
      REG_GPIP = 0x01,
      REG_IERA = 0x07,
      REG_IERB = 0x09,
      REG_IMRA = 0x13,
      REG_IMRB = 0x15,
      REG_TACR = 0x19,
      REG_TBCR = 0x1b,
      REG_TCDCR = 0x1d,
      REG_TADR = 0x1f,
      REG_TBDR = 0x21,
      REG_TCDR = 0x23,
      REG_TDDR = 0x25,
    };

    //! Timer/event channel indices
    enum Timer
    {
      TIMER_A = 0,
      TIMER_B = 1,
      TIMER_C = 2,
      TIMER_D = 3,
      GPI7 = 4,
      TIMERS_COUNT = 5,
    };

    explicit Mfp(Devices::M68K::Chip& core)
    {
      core.AddDevice(WINDOW_LO, WINDOW_HI, *this);
      Reset();
    }

    void Reset()
    {
      std::fill(std::begin(Regs), std::end(Regs), uint8_t(0));
      for (auto& timer : Timers)
      {
        timer = TimerState();
      }
      // The Atari OS keeps timer C enabled; it is harmless as the default handler is RTE
      Timers[TIMER_C].Enable = true;
      Timers[TIMER_C].Mask = true;
      // GPI7 is not a real timer; emulate a 1-event counter for the STe DMA end-of-frame
      Timers[GPI7].Control = 0x08;
      Timers[GPI7].DataInit = 1;
      Timers[GPI7].Data = 1;
      Clock.Reset();
      Clock.SetFrequency(CLOCK);
    }

    // Devices::M68K::Device
    uint8_t ReadByte(uint32_t addr) override
    {
      return Read(addr - WINDOW_LO);
    }

    uint16_t ReadWord(uint32_t addr) override
    {
      return uint16_t(0xff00 | Read(addr + 1 - WINDOW_LO));
    }

    void WriteByte(uint32_t addr, uint8_t data, const Devices::M68K::Oscillator& /*clock*/) override
    {
      Write(addr - WINDOW_LO, data);
    }

    void WriteWord(uint32_t addr, uint16_t data, const Devices::M68K::Oscillator& /*clock*/) override
    {
      Write(addr + 1 - WINDOW_LO, uint8_t(data));
    }

    uint8_t Read(uint_t reg) const
    {
      reg &= 0xff;
      if (0 == (reg & 1))
      {
        return 0xff;
      }
      switch (reg)
      {
      case REG_GPIP:
        // bit 7 is always set (mono detect), others are input-only
        return (Regs[reg] & 0x7f) | 0x80;
      case REG_TADR:
      case REG_TBDR:
      case REG_TCDR:
      case REG_TDDR:
        return Timers[(reg - REG_TADR) >> 1].Data;
      default:
        return Regs[reg];
      }
    }

    void Write(uint_t reg, uint8_t data)
    {
      reg &= 0xff;
      if (0 == (reg & 1))
      {
        return;
      }
      switch (reg)
      {
      case REG_TACR:
        Timers[TIMER_A].SetControl(data & 0x0f);
        break;
      case REG_TBCR:
        Timers[TIMER_B].SetControl(data & 0x0f);
        break;
      case REG_TCDCR:
        Timers[TIMER_C].SetControl((data >> 4) & 0x07);
        Timers[TIMER_D].SetControl(data & 0x07);
        break;
      case REG_TADR:
      case REG_TBDR:
      case REG_TCDR:
      case REG_TDDR:
        Timers[(reg - REG_TADR) >> 1].SetData(data);
        break;
      case REG_IERA:
        Timers[TIMER_A].SetEnable(0 != (data & (1 << 5)));
        Timers[TIMER_B].SetEnable(0 != (data & (1 << 0)));
        Timers[GPI7].SetEnable(0 != (data & (1 << 7)));
        break;
      case REG_IERB:
        Timers[TIMER_C].SetEnable(0 != (data & (1 << 5)));
        Timers[TIMER_D].SetEnable(0 != (data & (1 << 4)));
        break;
      case REG_IMRA:
        Timers[TIMER_A].Mask = 0 != (data & (1 << 5));
        Timers[TIMER_B].Mask = 0 != (data & (1 << 0));
        Timers[GPI7].Mask = 0 != (data & (1 << 7));
        break;
      case REG_IMRB:
        Timers[TIMER_C].Mask = 0 != (data & (1 << 5));
        Timers[TIMER_D].Mask = 0 != (data & (1 << 4));
        break;
      default:
        break;
      }
      Regs[reg] = data;
    }

    //! Queue an external event. The STe DMA end-of-frame is wired to timer A and GPI7 (as in
    //! AtariAudio Mk68901::SetSteDacExternalEvent); an event stays pending until a timer in event
    //! mode consumes it.
    void SignalExternalEvent()
    {
      Timers[TIMER_A].SetExternalEvent();
      Timers[GPI7].SetExternalEvent();
    }

    //! Number of handler invocations per timer since the last dispatch. The CPU runs in slices, so
    //! several timer ticks can pass before the queued handlers run; keep the count so a fast timer
    //! (e.g. a DMA sample feeder) keeps its rate instead of being slowed down by a busy main loop.
    using FiredTimers = std::array<uint_t, TIMERS_COUNT>;

    //! Consume pending external events by the event-mode timers
    FiredTimers ProcessExternalEvents()
    {
      FiredTimers fired{};
      for (uint_t idx = 0; idx != TIMERS_COUNT; ++idx)
      {
        if (Timers[idx].AdvanceEvent() && Timers[idx].Mask)
        {
          ++fired[idx];
        }
      }
      return fired;
    }

    //! Advance the counters up to the given machine time. Returns the per-timer number of fired
    //! (enabled and unmasked) events; the caller is responsible for dispatching their handlers.
    //! Deriving the tick delta from the oscillator avoids the drift of an explicit microsecond
    //! conversion.
    FiredTimers Synchronize(Time::AtMicrosecond now)
    {
      FiredTimers fired{};
      // GetTickAtTime wraps around for a non-positive delta, so only move strictly forward
      if (!(Clock.GetCurrentTime() < now))
      {
        return fired;
      }
      const auto delta = Clock.GetTickAtTime(now) - Clock.GetCurrentTick();
      Clock.AdvanceTick(delta);
      for (uint_t idx = 0; idx != TIMERS_COUNT; ++idx)
      {
        const auto count = Timers[idx].Advance(delta);
        if (count && Timers[idx].Mask)
        {
          fired[idx] += count;
        }
      }
      return fired;
    }

    //! Time until the closest scheduled timer event; a huge value when there is none
    Time::Microseconds TimeToNextEvent() const
    {
      uint64_t minTicks = std::numeric_limits<uint64_t>::max();
      for (const auto& timer : Timers)
      {
        if (timer.Mask)
        {
          minTicks = std::min(minTicks, timer.TicksToEvent());
        }
      }
      if (minTicks == std::numeric_limits<uint64_t>::max())
      {
        return Time::Microseconds(std::numeric_limits<uint64_t>::max());
      }
      // round up to make sure the event is not skipped when the duration is converted back to ticks;
      // never report a zero interval, otherwise a consumer that steps by it cannot make progress
      const uint64_t us = (minTicks * Time::Microsecond::PER_SECOND + CLOCK - 1) / CLOCK;
      return Time::Microseconds(std::max<uint64_t>(us, 1));
    }

    //! Program timer C to fire at (approximately) the given frequency and unmask its interrupt
    void SetTimerC(uint32_t frequency)
    {
      // The counter can only divide the clock by prescaler*data, so approximate the request with the
      // least-error pair
      uint32_t bestPrescale = 1;
      uint32_t bestData = 1;
      uint64_t bestErr = std::numeric_limits<uint64_t>::max();
      for (uint32_t p = 1; p != 8; ++p)
      {
        const uint32_t prescale = PRESCALE[p];
        const uint64_t scaled = uint64_t(frequency) * prescale;
        const uint64_t data = std::clamp<uint64_t>((CLOCK + scaled / 2) / scaled, 1, 255);
        const uint64_t actual = CLOCK / (prescale * data);
        const uint64_t err = actual > frequency ? actual - frequency : frequency - actual;
        if (err < bestErr)
        {
          bestErr = err;
          bestPrescale = p;
          bestData = uint32_t(data);
        }
      }
      Regs[REG_TCDR] = uint8_t(bestData);
      Regs[REG_TCDCR] = uint8_t((Regs[REG_TCDCR] & 0x0f) | (bestPrescale << 4));
      Regs[REG_IERB] |= 1 << 5;
      Regs[REG_IMRB] |= 1 << 5;
      auto& timer = Timers[TIMER_C];
      timer.SetControl(uint8_t(bestPrescale));
      timer.SetData(uint8_t(bestData));
      timer.Restart();
      timer.Enable = true;
      timer.Mask = true;
    }

  private:
    struct TimerState
    {
      uint8_t Control = 0;
      uint8_t Data = 0;
      uint8_t DataInit = 0;
      bool Enable = false;
      bool Mask = false;
      // External event queued by a device; consumed by event-mode timers
      bool ExternalEvent = false;
      // MFP clock ticks accumulated since the last decrement
      uint64_t InnerClock = 0;

      bool CounterMode() const
      {
        // bit 3 is event mode, bits 0..2 select the prescaler
        return 0 == (Control & 0x08) && 0 != PRESCALE[Control & 0x07];
      }

      void Restart()
      {
        InnerClock = 0;
        Data = DataInit;
      }

      void SetControl(uint8_t ctrl)
      {
        Control = ctrl;
      }

      void SetData(uint8_t data)
      {
        DataInit = data;
        if (0 == Control)
        {
          Restart();
        }
      }

      void SetEnable(bool enable)
      {
        // Enabling a stopped counter must restart it, otherwise it would resume with the stale count
        if (enable != Enable && enable && CounterMode())
        {
          Restart();
        }
        Enable = enable;
      }

      uint_t Advance(uint64_t ticks)
      {
        if (!Enable || !CounterMode())
        {
          return 0;
        }
        const auto divider = PRESCALE[Control & 0x07];
        InnerClock += ticks;
        uint_t fired = 0;
        while (InnerClock >= divider)
        {
          InnerClock -= divider;
          if (0 == --Data)
          {
            Data = DataInit;
            ++fired;
          }
        }
        return fired;
      }

      void SetExternalEvent()
      {
        ExternalEvent = true;
      }

      bool AdvanceEvent()
      {
        // Only event-mode timers (bit 3 set) react to external events
        if (!Enable || 0 == (Control & 0x08) || !ExternalEvent)
        {
          return false;
        }
        ExternalEvent = false;
        if (0 == --Data)
        {
          Data = DataInit;
          return true;
        }
        return false;
      }

      uint64_t TicksToEvent() const
      {
        if (!Enable || !CounterMode() || 0 == Data)
        {
          return std::numeric_limits<uint64_t>::max();
        }
        const auto divider = PRESCALE[Control & 0x07];
        const uint64_t total = uint64_t(Data) * divider;
        return total > InnerClock ? total - InnerClock : 0;
      }
    };

  private:
    TimerState Timers[TIMERS_COUNT];
    uint8_t Regs[256] = {};
    // MFP ticks are derived from the machine timeline through a dedicated oscillator
    Time::MicrosecOscillator Clock;
  };

  //! Video shifter stub: SNDH drivers query the resolution and the PAL/NTSC sync registers
  class Shifter : public Devices::M68K::Device
  {
  public:
    //! Register window
    static const uint32_t WINDOW_LO = 0xff8200;
    static const uint32_t WINDOW_HI = WINDOW_LO + 0x100;

    //! Registers (byte offsets within WINDOW_LO)
    enum Register
    {
      REG_SYNC = 0x0a,
      REG_RESOLUTION = 0x60,
    };

    explicit Shifter(Devices::M68K::Chip& core)
    {
      core.AddDevice(WINDOW_LO, WINDOW_HI, *this);
    }

    uint8_t ReadByte(uint32_t addr) override
    {
      switch (addr - WINDOW_LO)
      {
      case REG_RESOLUTION:
        // Video resolution register: report low resolution
        return 0;
      case REG_SYNC:
        // Video sync register: report PAL (50Hz)
        return 2;
      default:
        return 0xff;
      }
    }

    uint16_t ReadWord(uint32_t addr) override
    {
      return uint16_t(0xff00 | ReadByte(addr + 1));
    }

    void WriteByte(uint32_t /*addr*/, uint8_t /*data*/, const Devices::M68K::Oscillator& /*clock*/) override {}

    void WriteWord(uint32_t /*addr*/, uint16_t /*data*/, const Devices::M68K::Oscillator& /*clock*/) override {}
  };

  //! Sink for hardware events raised by peripherals (e.g. the STe DMA end-of-buffer interrupt);
  //! the machine latches them and dispatches the corresponding MFP timer handlers synchronously.
  class ExternalEventSink
  {
  public:
    virtual ~ExternalEventSink() = default;

    virtual void SignalExternalEvent() = 0;
  };

  //! LMC1992 digitally controlled stereo audio processor, used in the Atari STe for volume and tone
  //! control. A pluggable bus Device serving the Microwire window [0xFF8922, 0xFF8926). It knows
  //! nothing about audio rendering: the renderer reads its coefficients and uses them to mix
  //! the mono YM2149 output with the stereo STe DMA stream.
  class Lmc1992 : public Devices::M68K::Device
  {
  public:
    //! Microwire register window
    static const uint32_t WINDOW_LO = 0xff8922;
    static const uint32_t WINDOW_HI = WINDOW_LO + 4;

    //! Registers: two 16-bit big-endian values (byte offsets within WINDOW_LO)
    enum Register
    {
      REG_DATA_HI = 0x00,
      REG_DATA_LO = 0x01,
      REG_MASK_HI = 0x02,
      REG_MASK_LO = 0x03,
    };

    //! Stereo gains applied to the mono YM2149 and the stereo DMA contributions (0 dB == 1.0)
    struct Coefficients
    {
      Sound::Gain Ym;
      Sound::Gain Dac;
    };

    explicit Lmc1992(Devices::M68K::Chip& core)
    {
      core.AddDevice(WINDOW_LO, WINDOW_HI, *this);
    }

    uint8_t ReadByte(uint32_t addr) override
    {
      switch (addr - WINDOW_LO)
      {
      case REG_DATA_LO:
        return uint8_t(MicrowireData);
      case REG_MASK_LO:
        return uint8_t(MicrowireMask);
      default:
        return 0xff;
      }
    }

    uint16_t ReadWord(uint32_t addr) override
    {
      switch (addr - WINDOW_LO)
      {
      case REG_DATA_HI:
        return MicrowireData;
      case REG_MASK_HI:
        return TickMask();
      default:
        return 0xffff;
      }
    }

    void WriteByte(uint32_t addr, uint8_t data, const Time::MicrosecOscillator& /*clock*/) override
    {
      switch (addr - WINDOW_LO)
      {
      case REG_DATA_HI:
        MicrowireData = uint16_t((MicrowireData & 0x00ff) | (uint16_t(data) << 8));
        break;
      case REG_DATA_LO:
        MicrowireData = uint16_t((MicrowireData & 0xff00) | data);
        break;
      case REG_MASK_HI:
        MicrowireMask = uint16_t((MicrowireMask & 0x00ff) | (uint16_t(data) << 8));
        break;
      case REG_MASK_LO:
        MicrowireMask = uint16_t((MicrowireMask & 0xff00) | data);
        break;
      default:
        break;
      }
    }

    void WriteWord(uint32_t addr, uint16_t data, const Time::MicrosecOscillator& /*clock*/) override
    {
      switch (addr - WINDOW_LO)
      {
      case REG_DATA_HI:
        MicrowireData = data;
        MicrowireShift = 16;
        Proceed();
        break;
      case REG_MASK_HI:
        MicrowireMask = data;
        break;
      default:
        break;
      }
    }

    //! Coefficients for the currently programmed state (recomputed lazily)
    Coefficients GetCoefficients() const
    {
      UpdateCoefficients();
      return Coefs;
    }

    //! Input select (0..3): 0 = DMA+YM -12dB, 1 = DMA+YM, 2 = DMA only, 3 = reserved
    uint_t MixMode() const
    {
      return Mix;
    }

    uint_t MasterVolume() const
    {
      return Master;
    }

    uint_t LeftVolume() const
    {
      return Left;
    }

    uint_t RightVolume() const
    {
      return Right;
    }

    uint_t BassLevel() const
    {
      return Bass;
    }

    uint_t TrebleLevel() const
    {
      return Treble;
    }

  private:
    void Proceed()
    {
      // Only the bits set in the mask are clocked out, so pack them in mask order into the frame
      uint16_t value = 0;
      uint_t count = 0;
      for (uint_t idx = 0; idx != 16; ++idx)
      {
        if (MicrowireMask & (1 << idx))
        {
          if (MicrowireData & (1 << idx))
          {
            value |= (1 << count);
          }
          ++count;
        }
      }
      // LMC1992 frames are 11 bits long and addressed with '10'
      if (11 != count || 2 != (value >> 9))
      {
        return;
      }
      const uint_t data = value & 0x3f;
      switch ((value >> 6) & 7)
      {
      case 0:  // input select
        Mix = data & 3;
        break;
      case 1:  // bass
        Bass = std::min<uint_t>(data & 0x0f, 12);
        break;
      case 2:  // treble
        Treble = std::min<uint_t>(data & 0x0f, 12);
        break;
      case 3:  // master volume: 0..40 -> -80..0 dB
        Master = std::min<uint_t>(data, 40);
        break;
      case 4:  // right volume: 0..20 -> -40..0 dB
        Right = std::min<uint_t>(data & 0x1f, 20);
        break;
      case 5:  // left volume: 0..20 -> -40..0 dB
        Left = std::min<uint_t>(data & 0x1f, 20);
        break;
      default:
        return;
      }
      Dirty = true;
      Dbg("LMC mix={} master={} left={} right={} bass={} treble={}", Mix, Master, Left, Right, Bass, Treble);
    }

    // emulate the internal rol.w #1 so that code polling the shift register sees the transmission
    uint16_t TickMask()
    {
      if (MicrowireShift > 0)
      {
        MicrowireMask = uint16_t((MicrowireMask << 1) | (MicrowireMask >> 15));
        --MicrowireShift;
      }
      return MicrowireMask;
    }

    //! Attenuation of the given number of 2 dB steps as a 0 dB == 1.0 gain
    static Sound::Gain::Type Attenuation(uint_t stepsDown)
    {
      const auto value = std::lround(std::pow(10.0, -double(stepsDown) / 10.0) * 256.0);
      return Sound::Gain::Type(int_t(value), 256);
    }

    void UpdateCoefficients() const
    {
      if (!Dirty)
      {
        return;
      }
      const auto gMaster = Attenuation(40 - Master);
      const auto gLeft = gMaster * Attenuation(20 - Left);
      const auto gRight = gMaster * Attenuation(20 - Right);
      const bool mixYm = 0 == Mix || 1 == Mix;
      // Mode 0 selects the DMA+YM mix 12 dB below mode 1 (datasheet; broken on real STe, equals mode 1)
      const auto dacGain = 0 == Mix ? Attenuation(6) : Sound::Gain::Type(1, 1);
      Coefs.Ym = Sound::Gain{mixYm ? gLeft : Sound::Gain::Type(), mixYm ? gRight : Sound::Gain::Type()};
      Coefs.Dac = Sound::Gain{dacGain * gLeft, dacGain * gRight};
      Dirty = false;
    }

  private:
    uint16_t MicrowireData = 0;
    uint16_t MicrowireMask = 0;
    uint16_t MicrowireShift = 0;
    // Hardware reset defaults: YM+DMA mix at 0 dB, flat tone
    uint_t Mix = 1;
    uint_t Master = 40;
    uint_t Left = 20;
    uint_t Right = 20;
    uint_t Bass = 6;
    uint_t Treble = 6;
    mutable bool Dirty = true;
    mutable Coefficients Coefs;
  };

  // STe DMA sound base frequency and the four replay rates selected by the mode register
  const uint32_t STE_DAC_FREQ = 50066;
  const uint32_t STE_DAC_RATES[] = {STE_DAC_FREQ / 8, STE_DAC_FREQ / 4, STE_DAC_FREQ / 2, STE_DAC_FREQ};
  // 0 dB LMC reference level: one 8-bit DMA sample is scaled into +-64 of the 14-bit output domain
  const int DAC_UNIT = 64;

  //! STe DMA sound replay, mapped at 0xFF8900. A pluggable bus Device driven once per host output
  //! sample by the renderer. It owns the DMA register file, the sample pointer and the rate
  //! conversion, and reports the produced stereo levels at the 0 dB LMC1992 reference. The
  //! end-of-buffer event is signaled back to the machine.
  class SteDma : public Devices::M68K::Device
  {
  public:
    //! Register window
    static const uint32_t WINDOW_LO = 0xff8900;
    static const uint32_t WINDOW_HI = 0xff8922;

    //! Registers (byte offsets within WINDOW_LO; the addresses are 16-bit, big-endian)
    enum Register
    {
      REG_CONTROL = 0x01,
      REG_START_HI = 0x03,
      REG_START_MID = 0x05,
      REG_START_LO = 0x07,
      REG_COUNTER_HI = 0x09,
      REG_COUNTER_MID = 0x0b,
      REG_COUNTER_LO = 0x0d,
      REG_END_HI = 0x0f,
      REG_END_MID = 0x11,
      REG_END_LO = 0x13,
      REG_MODE = 0x21,
    };

    //! Receiver of the per-sample stereo levels; the LMC1992 mix is applied downstream
    class Sink
    {
    public:
      virtual ~Sink() = default;

      virtual void AddDmaSample(int16_t left, int16_t right) = 0;
    };

    SteDma(Devices::M68K::Chip& core, ExternalEventSink& events, uint_t hostRate)
      : Core(core)
      , Events(events)
      , HostRate(hostRate)
    {
      core.AddDevice(WINDOW_LO, WINDOW_HI, *this);
      Reset();
    }

    //! Return to the power-on state; the host rate is kept
    void Reset()
    {
      std::fill(std::begin(Regs), std::end(Regs), uint8_t(0));
      SamplePtr = 0;
      SampleStartPtr = 0;
      SampleEndPtr = 0;
      InnerClock = 0;
      LevelL = 0;
      LevelR = 0;
      PendingEvent = false;
      LastMode = 0xff;
    }

    void SetSink(Sink* sink)
    {
      SampleSink = sink;
    }

    uint8_t ReadByte(uint32_t addr) override
    {
      return Read8(addr - WINDOW_LO);
    }

    uint16_t ReadWord(uint32_t addr) override
    {
      return Read16(addr - WINDOW_LO);
    }

    void WriteByte(uint32_t addr, uint8_t data, const Time::MicrosecOscillator& /*clock*/) override
    {
      Write8(addr - WINDOW_LO, data);
    }

    void WriteWord(uint32_t addr, uint16_t data, const Time::MicrosecOscillator& /*clock*/) override
    {
      Write16(addr - WINDOW_LO, data);
    }

    //! Advance the replay by one host output sample
    void ProcessSample()
    {
      Step();
      if (SampleSink)
      {
        SampleSink->AddDmaSample(LevelL, LevelR);
      }
      if (PendingEvent)
      {
        PendingEvent = false;
        Events.SignalExternalEvent();
      }
    }

  private:
    uint8_t Read8(uint_t off) const
    {
      off &= 0xff;
      if (0 == (off & 1))
      {
        return 0xff;
      }
      switch (off)
      {
      case REG_COUNTER_HI:
        return uint8_t(SamplePtr >> 16);
      case REG_COUNTER_MID:
        return uint8_t(SamplePtr >> 8);
      case REG_COUNTER_LO:
        return uint8_t(SamplePtr);
      default:
        return Regs[off];
      }
    }

    uint16_t Read16(uint_t off) const
    {
      off &= 0xff;
      if (off & 1)
      {
        return 0xffff;
      }
      return uint16_t(0xff00 | Read8(off + 1));
    }

    void Write8(uint_t off, uint8_t data)
    {
      off &= 0xff;
      if (0 == (off & 1))
      {
        return;
      }
      switch (off)
      {
      case REG_CONTROL:
        // The control bit 0 rising edge is a replay (re)start, so reload the sample pointers
        if ((data & 1) && (0 != ((data ^ Regs[REG_CONTROL]) & 1)))
        {
          FetchSamplePtr();
        }
        break;
      case REG_START_LO:
      case REG_COUNTER_LO:
        data &= 0xfe;
        break;
      default:
        break;
      }
      Regs[off] = data;
      if (REG_MODE == off && data != LastMode)
      {
        LastMode = data;
        Dbg("DMA mode=${:02x} rate={} {}", data, data & 3, (data & 0x80) ? "mono" : "stereo");
      }
    }

    void Write16(uint_t off, uint16_t data)
    {
      off &= 0xff;
      if (off & 1)
      {
        return;
      }
      Write8(off + 1, uint8_t(data));
    }

    //! Advance the replay by one host sample, producing the stereo level at the 0 dB LMC reference
    void Step()
    {
      if (0 == (Regs[REG_CONTROL] & 1))
      {
        LevelL = LevelR = 0;
        return;
      }
      const bool stereo = 0 == (Regs[REG_MODE] & 0x80);
      const uint_t frameSize = stereo ? 2 : 1;
      InnerClock += STE_DAC_RATES[Regs[REG_MODE] & 3];
      while (InnerClock >= HostRate)
      {
        InnerClock -= HostRate;
        SamplePtr += frameSize;
        if (SamplePtr == SampleEndPtr)
        {
          PendingEvent = true;
          FetchSamplePtr();
          if (0 == (Regs[REG_CONTROL] & (1 << 1)))
          {
            // In no-loop mode the replay stops at the buffer end instead of restarting
            Regs[REG_CONTROL] &= 0xfe;
            LevelL = LevelR = 0;
            InnerClock = 0;
            return;
          }
        }
      }
      // The mode sets the frame rate for both mono and stereo (stereo consumes two bytes per
      // frame), so the same fractional interpolation serves every mode. It replaces the mode-3
      // pair averaging: that made a 25kHz stream, while the hardware keeps fetching at 50kHz.
      const int curL = Fetch(SamplePtr);
      const int curR = stereo ? Fetch(SamplePtr + 1) : curL;
      int nxtL = curL;
      int nxtR = curR;
      if (SamplePtr + frameSize < SampleEndPtr)
      {
        nxtL = Fetch(SamplePtr + frameSize);
        nxtR = stereo ? Fetch(SamplePtr + frameSize + 1) : nxtL;
      }
      else if (Regs[REG_CONTROL] & (1 << 1))
      {
        // The hardware reloads the base address at the end, so the loop continues from there
        nxtL = Fetch(SampleStartPtr);
        nxtR = stereo ? Fetch(SampleStartPtr + 1) : nxtL;
      }
      const int32_t frac = int32_t(InnerClock);
      const int32_t scale = int32_t(HostRate);
      LevelL = int16_t(Interpolate(curL, nxtL, scale - frac, frac, scale) * DAC_UNIT);
      LevelR = int16_t(Interpolate(curR, nxtR, scale - frac, frac, scale) * DAC_UNIT);
    }

    static int Interpolate(int from, int to, int32_t fromWeight, int32_t toWeight, int32_t scale)
    {
      const int32_t sum = int32_t(from) * fromWeight + int32_t(to) * toWeight;
      const int32_t half = (sum < 0 ? -scale : scale) / 2;
      return (sum + half) / scale;
    }

    void FetchSamplePtr()
    {
      SamplePtr = (uint32_t(Regs[REG_START_HI]) << 16) | (uint32_t(Regs[REG_START_MID]) << 8)
                  | (Regs[REG_START_LO] & 0xfe);
      SampleStartPtr = SamplePtr;
      SampleEndPtr = (uint32_t(Regs[REG_END_HI]) << 16) | (uint32_t(Regs[REG_END_MID]) << 8)
                     | (Regs[REG_END_LO] & 0xfe);
    }

    int8_t Fetch(uint32_t addr) const
    {
      return int8_t(Core.ReadRamByte(addr));
    }

  private:
    Devices::M68K::Chip& Core;
    ExternalEventSink& Events;
    const uint_t HostRate;
    Sink* SampleSink = nullptr;
    uint8_t Regs[256] = {};
    uint32_t SamplePtr = 0;
    uint32_t SampleStartPtr = 0;
    uint32_t SampleEndPtr = 0;
    uint32_t InnerClock = 0;
    int16_t LevelL = 0;
    int16_t LevelR = 0;
    bool PendingEvent = false;
    uint8_t LastMode = 0xff;
  };

  //! STe blitter (BLiTTER) register block at 0xFF8A00: a RAM-to-RAM transfer engine with shift,
  //! end-masks, 16 logical ops and a halftone pattern. A pluggable bus device; it latches a register
  //! file and runs the transfer to completion when the control register's busy bit is written set,
  //! reading and writing machine RAM through the owner. Ported from AtariAudio (sndh-player)
  //! src/Blitter.cpp by Arnaud Carré aka Leonard/Oxygene (@leonard_coder).
  class Blitter : public Devices::M68K::Device
  {
  public:
    static const uint32_t WINDOW_LO = 0xff8a00;
    static const uint32_t WINDOW_HI = WINDOW_LO + 0x40;

    //! Registers (byte offsets within WINDOW_LO)
    enum Register
    {
      REG_SRC_X_INC = 0x20,
      REG_SRC_Y_INC = 0x22,
      REG_SRC_HI = 0x24,
      REG_SRC_LO = 0x26,
      REG_END_MASK_1 = 0x28,
      REG_END_MASK_2 = 0x2a,
      REG_END_MASK_3 = 0x2c,
      REG_DST_X_INC = 0x2e,
      REG_DST_Y_INC = 0x30,
      REG_DST_HI = 0x32,
      REG_DST_LO = 0x34,
      REG_X_COUNT = 0x36,
      REG_Y_COUNT = 0x38,
      REG_HOP = 0x3a,
      REG_OP = 0x3b,
      REG_CONTROL = 0x3c,
      REG_SKEW = 0x3d,
    };

    explicit Blitter(Devices::M68K::Chip& core)
      : Core(core)
    {
      core.AddDevice(WINDOW_LO, WINDOW_HI, *this);
      Reset();
    }

    //! Return to the power-on state
    void Reset()
    {
      std::fill(std::begin(Regs), std::end(Regs), uint8_t(0));
    }

    uint8_t ReadByte(uint32_t addr) override
    {
      return Regs[(addr - WINDOW_LO) & 0x3f];
    }

    uint16_t ReadWord(uint32_t addr) override
    {
      const uint_t reg = (addr - WINDOW_LO) & 0x3f;
      return uint16_t((uint16_t(Regs[reg]) << 8) | Regs[(reg + 1) & 0x3f]);
    }

    //! Registers latch immediately; a write to the control register runs the transfer to completion,
    //! so the CPU never observes the blitter busy
    void WriteByte(uint32_t addr, uint8_t data, const Time::MicrosecOscillator& /*clock*/) override
    {
      const uint_t reg = (addr - WINDOW_LO) & 0x3f;
      Regs[reg] = data;
      if (REG_CONTROL == reg)
      {
        Run();
      }
    }

    void WriteWord(uint32_t addr, uint16_t data, const Time::MicrosecOscillator& /*clock*/) override
    {
      const uint_t reg = (addr - WINDOW_LO) & 0x3f;
      Regs[reg] = uint8_t(data >> 8);
      Regs[(reg + 1) & 0x3f] = uint8_t(data);
      if (REG_CONTROL == reg)
      {
        Run();
      }
    }

  private:
    uint16_t Reg16(uint_t off) const
    {
      return uint16_t((uint16_t(Regs[off & 0x3f]) << 8) | Regs[(off + 1) & 0x3f]);
    }

    void SetReg16(uint_t off, uint16_t value)
    {
      Regs[off & 0x3f] = uint8_t(value >> 8);
      Regs[(off + 1) & 0x3f] = uint8_t(value);
    }

    //! Logical ops 0/5/10/15 ignore the source, so the source fetch is skipped for them
    uint16_t ReadHop()
    {
      static const int SourceRead[16] = {0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0};
      uint16_t hop = 0;
      if (SourceRead[Regs[REG_OP] & 0x0f])
      {
        switch (Regs[REG_HOP] & 3)
        {
        case 0:
          hop = 0xffff;
          break;
        case 1:
          hop = Reg16(HalfToneLine << 1);
          break;
        case 2:
          hop = SourceWord();
          break;
        case 3:
          hop = SourceWord() & Reg16(HalfToneLine << 1);
          break;
        default:
          break;
        }
      }
      return hop;
    }

    uint16_t SourceWord()
    {
      if (NFSR && 1 == XCount)
      {
        // Reference quirk: on the last word of an NFSR line keep the motif and back the source up
        const auto word = uint16_t(CurrentMotif);
        SrcAd -= XSrcInc;
        return word;
      }
      const auto word = Core.ReadRamWord(SrcAd);
      if (XCount > 1)
      {
        SrcAd += XSrcInc;
      }
      return word;
    }

    void InternalFetch()
    {
      CurrentMotif = (CurrentMotif << 16) | ReadHop();
    }

    void Run()
    {
      if (0 == (Regs[REG_CONTROL] & 0x80))
      {
        return;
      }
      int lineCount = Reg16(REG_Y_COUNT);
      if (lineCount > 0)
      {
        CurrentMotif = 0;
        XCountReset = 0 == Reg16(REG_X_COUNT) ? 65536 : Reg16(REG_X_COUNT);
        XSrcInc = int16_t(Reg16(REG_SRC_X_INC) & 0xfffe);
        YSrcInc = int16_t(Reg16(REG_SRC_Y_INC) & 0xfffe);
        XDstInc = int16_t(Reg16(REG_DST_X_INC) & 0xfffe);
        YDstInc = int16_t(Reg16(REG_DST_Y_INC) & 0xfffe);
        const auto shift = Regs[REG_SKEW] & 0x0f;
        FXSR = 0 != (Regs[REG_SKEW] & 0x80);
        NFSR = 0 != (Regs[REG_SKEW] & 0x40);
        const auto logicalOp = Regs[REG_OP] & 0x0f;
        SrcAd = ((uint32_t(Reg16(REG_SRC_HI)) << 16) | Reg16(REG_SRC_LO)) & 0x00fffffe;
        DstAd = ((uint32_t(Reg16(REG_DST_HI)) << 16) | Reg16(REG_DST_LO)) & 0x00fffffe;
        const uint16_t endMask[3] = {Reg16(REG_END_MASK_1), Reg16(REG_END_MASK_2), Reg16(REG_END_MASK_3)};
        HalfToneLine = Regs[REG_CONTROL] & 0x0f;

        // A transfer cannot usefully address more words than the 4 Mb RAM holds; bound it so a corrupt
        // register setup cannot spin the host forever (the blitter runs outside the CPU budget)
        uint64_t budget = 0x200000;
        do
        {
          XCount = XCountReset;
          if (FXSR)
          {
            InternalFetch();
          }
          int maskId = 0;
          while (XCount >= 1)
          {
            if (0 == budget--)
            {
              lineCount = 0;
              break;
            }
            InternalFetch();
            const uint16_t hop = 1 != Regs[REG_HOP] ? uint16_t(CurrentMotif >> shift) : uint16_t(CurrentMotif);
            const uint16_t original = Core.ReadRamWord(DstAd);
            const uint16_t combined = Combine(logicalOp, hop, original);
            Core.WriteRamWord(DstAd, uint16_t((combined & endMask[maskId]) | (original & ~endMask[maskId])));
            if (XCount > 1)
            {
              DstAd += XDstInc;
            }
            --XCount;
            if (0 == maskId)
            {
              maskId = 1;
            }
            if (1 == XCount)
            {
              maskId = 2;
            }
          }
          SrcAd += YSrcInc;
          DstAd += YDstInc;
          HalfToneLine = YDstInc >= 0 ? (HalfToneLine + 1) & 15 : (HalfToneLine - 1) & 15;
        } while (--lineCount);

        SetReg16(REG_SRC_HI, uint16_t(SrcAd >> 16));
        SetReg16(REG_SRC_LO, uint16_t(SrcAd & 0xfffe));
        SetReg16(REG_DST_HI, uint16_t(DstAd >> 16));
        SetReg16(REG_DST_LO, uint16_t(DstAd & 0xfffe));
        SetReg16(REG_Y_COUNT, 0);
      }
      Regs[REG_CONTROL] &= 0x7f;
    }

    static uint16_t Combine(int logicalOp, uint16_t hop, uint16_t dst)
    {
      switch (logicalOp)
      {
      case 0:
        return 0;
      case 1:
        return hop & dst;
      case 2:
        return hop & ~dst;
      case 3:
        return hop;
      case 4:
        return ~hop & dst;
      case 5:
        return dst;
      case 6:
        return hop ^ dst;
      case 7:
        return hop | dst;
      case 8:
        return ~hop & ~dst;
      case 9:
        return ~hop ^ dst;
      case 10:
        return ~dst;
      case 11:
        return hop | ~dst;
      case 12:
        return ~hop;
      case 13:
        return ~hop | dst;
      case 14:
        return ~hop | ~dst;
      default:
        return 0xffff;
      }
    }

    Devices::M68K::Chip& Core;
    uint8_t Regs[0x40] = {};
    uint32_t CurrentMotif = 0;
    int XCountReset = 0;
    int XCount = 0;
    uint32_t SrcAd = 0;
    uint32_t DstAd = 0;
    int16_t XSrcInc = 0;
    int16_t YSrcInc = 0;
    int16_t XDstInc = 0;
    int16_t YDstInc = 0;
    int HalfToneLine = 0;
    bool FXSR = false;
    bool NFSR = false;
  };

  //! Routes the machine bus accesses in the YM2149 window to the AY data channel. Inherits the
  //! channel so the renderer drives it directly while the bus sees only a peripheral.
  class AtariAyDevice
    : public Devices::M68K::Device
    , public AyDataChannel
  {
  public:
    //! Register window
    static const uint32_t WINDOW_LO = 0xff8800;
    static const uint32_t WINDOW_HI = 0xff8900;

    //! Registers: address bit 1 selects the data port over the register select
    enum Register
    {
      REG_SELECT = 0x00,
      REG_DATA = 0x02,
    };

    AtariAyDevice(Devices::M68K::Chip& core, Devices::AYM::Chip::Ptr chip)
      : AyDataChannel(std::move(chip))
    {
      core.AddDevice(WINDOW_LO, WINDOW_HI, *this);
    }

    uint8_t ReadByte(uint32_t /*addr*/) override
    {
      return GetValue();
    }

    uint16_t ReadWord(uint32_t /*addr*/) override
    {
      return uint16_t(GetValue()) << 8;
    }

    void WriteByte(uint32_t addr, uint8_t data, const Time::MicrosecOscillator& clock) override
    {
      if (addr & REG_DATA)
      {
        SetValue(clock.GetCurrentTime(), data);
      }
      else
      {
        SelectRegister(data);
      }
    }

    void WriteWord(uint32_t addr, uint16_t data, const Time::MicrosecOscillator& clock) override
    {
      const auto byte = uint8_t(data >> 8);
      if (addr & REG_DATA)
      {
        SetValue(clock.GetCurrentTime(), byte);
      }
      else
      {
        SelectRegister(byte);
      }
    }
  };

  //! Atari ST machine: the generic 68k core plus the Atari chips (MFP, shifter stub, sound chip,
  //! STe DMA, LMC1992, blitter), the SNDH exception/trap/vector setup and the MFP-timer-C playback
  //! model. Owns every device, which register themselves on the bus; the machine dispatches their
  //! external events.
  class Machine
    : public Devices::M68K::CycleObserver
    , public ExternalEventSink
  {
  public:
    Machine(Devices::AYM::Chip::Ptr ay, uint_t dacRate)
      : Core(Devices::M68K::CreateChip(CPU_CLOCK, RAM_SIZE))
      , DacRate(dacRate)
      , MfpState(*Core)
      , ShifterState(*Core)
      , AyState(*Core, std::move(ay))
      , DmaState(*Core, *this, dacRate)
      , LmcState(*Core)
      , BlitState(*Core)
    {
      Reset();
    }

    void Reset()
    {
      Core->Reset();
      MfpState.Reset();
      DacClock.Reset();
      DacClock.SetFrequency(DacRate, DacRate);
      PendingTimers = {};

      // Any uninstalled exception/trap vector points here and returns via RTE
      const uint8_t rte[] = {0x4e, 0x73};
      Core->WriteBlock(0, rte);

      // TRAP #14 -> vector 0x80 + 14 * 4 = 0xb8 (front stub answers Blitmode, then jumps to api68)
      Core->WriteBlock(TRAP14_ADDR, TRAP14);
      Core->WriteBlock(TRAP14_FRONT_ADDR, TRAP14_FRONT);
      Core->WriteDword(0xb8, TRAP14_FRONT_ADDR);
      // TRAP #1 -> vector 0x80 + 1 * 4 = 0x84
      Core->WriteBlock(GEMDOS_ADDR, GEMDOS);
      Core->WriteDword(0x84, GEMDOS_ADDR);
      Core->WriteDword(GEMDOS_NEXT_ADDR, GEMDOS_MALLOC_START);
      // Timer C default handler is RTE (unset => skipped)
      Core->WriteDword(TIMER_VECTORS[Mfp::TIMER_C], 0);

      // Cookie jar queried by Maxymizer and similar drivers
      Core->WriteBlock(COOKIE_JAR_ADDR, COOKIE_JAR);
      Core->WriteDword(0x5a0, COOKIE_JAR_ADDR);  // cookie jar start
    }

    void Load(Binary::View data)
    {
      Core->Load(UPLOAD_ADDR, data);
    }

    //! Run the driver init for the (1-based) subsong, then install the play routine as the MFP
    //! timer C handler firing at the given frequency and prime the sound registers. The main code
    //! is left halted; the play routine runs from the interrupt handlers while Advance() executes.
    void Init(uint_t subsong, uint32_t frequency)
    {
      Core->SetFaulted(false);
      Core->ExecuteSubroutine(UPLOAD_ADDR, subsong, INIT_CYCLES, nullptr);
      if (Core->Faulted())
      {
        return;
      }
      // Route the play routine through the MFP timer C interrupt (the default SNDH timer): the
      // handler calls the play entry point and returns from the interrupt
      Core->WriteBlock(PLAY_STUB_ADDR, PLAY_STUB);
      Core->WriteDword(TIMER_VECTORS[Mfp::TIMER_C], PLAY_STUB_ADDR);
      MfpState.SetTimerC(frequency);
      // Set the initial sound registers before the first interrupt fires
      Core->ExecuteSubroutine(UPLOAD_ADDR + 8, 0, CALL_CYCLES, nullptr);
    }

    Time::AtMicrosecond GetTime() const
    {
      return Core->GetTime();
    }

    Time::Microseconds TimeToNextEvent() const
    {
      return MfpState.TimeToNextEvent();
    }

    AtariAyDevice& Ay()
    {
      return AyState;
    }

    SteDma& Dma()
    {
      return DmaState;
    }

    Lmc1992& Lmc()
    {
      return LmcState;
    }

    //! Produce the DMA samples up to the given machine time. Re-entrant calls (a DMA event handler
    //! dispatched from ProcessSample) are ignored; the outer clocking loop continues afterwards.
    void ClockDac(Time::AtMicrosecond till)
    {
      if (DacBusy)
      {
        return;
      }
      DacBusy = true;
      const uint_t count = DacClock.HasSamplesBefore(till) ? DacClock.SamplesTill(till) : 0;
      for (uint_t idx = 0; idx != count; ++idx)
      {
        DmaState.ProcessSample();
      }
      DacClock.CommitSamples(count);
      DacBusy = false;
    }

    //! Notify the machine about an external event and dispatch the event-mode MFP timers
    void SignalExternalEvent() override
    {
      MfpState.SignalExternalEvent();
      if (!Core->Faulted())
      {
        const auto fired = MfpState.ProcessExternalEvents();
        for (uint_t idx = 0; idx != Mfp::TIMERS_COUNT; ++idx)
        {
          for (uint_t event = 0; event != fired[idx]; ++event)
          {
            Core->ExecuteInterrupt(TIMER_VECTORS[idx], CALL_CYCLES, this);
          }
        }
        // Counter timers that fired while the event handlers ran
        DispatchTimers();
      }
    }

    void Advance(Time::Microseconds delta)
    {
      const auto target = GetTime() + delta;
      while (!Core->Faulted())
      {
        const auto now = GetTime();
        if (!(now < target))
        {
          break;
        }
        // Timers that fired while a handler was running were queued by the per-slice sync
        if (HasPendingTimers())
        {
          DispatchTimers();
          continue;
        }
        auto step = target - now;
        const auto toEvent = MfpState.TimeToNextEvent();
        if (toEvent < step)
        {
          step = toEvent;
        }
        // The oscillator is the single timeline: move it to the sub-step end before dispatching the
        // handlers, so that the output time and the IO stamps stay ordered
        const auto stop = now + step;
        Core->SetTime(stop);
        AddPendingTimers(MfpState.Synchronize(stop));
        DispatchTimers();
        if (0 == step.Get() && !HasPendingTimers())
        {
          break;
        }
      }
      // A faulted engine produces silence but must not stall the output timeline
      if (GetTime() < target)
      {
        Core->SetTime(target);
      }
      ClockDac(GetTime());
    }

    // Devices::M68K::CycleObserver
    void OnCycles(uint32_t /*cycles*/) override
    {
      // Catch the MFP clock up to the current output time after every CPU slice, so the timer clock
      // never lags the master clock during a handler
      AddPendingTimers(MfpState.Synchronize(GetTime()));
    }

  private:
    //! A long main routine can queue many timer ticks, so the backlog is drained on the next
    //! dispatch. The bound prevents a runaway loop when a handler cannot keep up with its timer
    //! (e.g. a DMA feeder that is slower than the requested rate).
    static constexpr uint_t MAX_PENDING_PER_TIMER = 128;

    void AddPendingTimers(const Mfp::FiredTimers& fired)
    {
      for (uint_t idx = 0; idx != Mfp::TIMERS_COUNT; ++idx)
      {
        PendingTimers[idx] = std::min<uint_t>(PendingTimers[idx] + fired[idx], MAX_PENDING_PER_TIMER);
      }
    }

    bool HasPendingTimers() const
    {
      return std::any_of(PendingTimers.begin(), PendingTimers.end(), [](uint_t count) { return count != 0; });
    }

    //! Run the handlers of the queued timers; each timer may have fired several times, each fire is
    //! dispatched separately. Handlers may queue more timers, they run on the next dispatch.
    void DispatchTimers()
    {
      const auto fired = PendingTimers;
      PendingTimers = {};
      for (uint_t idx = 0; idx != Mfp::TIMERS_COUNT; ++idx)
      {
        for (uint_t event = 0; event != fired[idx]; ++event)
        {
          // Clock the DMA up to the handler start, then again after it, so the samples between
          // handlers use the buffer state produced by the corresponding handler
          ClockDac(GetTime());
          // The player tick (timer C) is a synchronous update: its CPU time must not consume
          // output time, otherwise a long routine holds the DMA buffer and distorts the stream
          Core->ExecuteInterrupt(TIMER_VECTORS[idx], CALL_CYCLES, idx == Mfp::TIMER_C ? nullptr : this);
        }
      }
      ClockDac(GetTime());
    }

  private:
    Devices::M68K::Chip::Ptr Core;
    const uint_t DacRate;
    Mfp MfpState;
    Shifter ShifterState;
    AtariAyDevice AyState;
    SteDma DmaState;
    Lmc1992 LmcState;
    Blitter BlitState;
    Devices::Details::ClockSource<Time::AtMicrosecond> DacClock;
    bool DacBusy = false;
    // Fired timer counts queued by the per-slice sync, waiting for their handlers to be dispatched
    Mfp::FiredTimers PendingTimers{};
  };

  //! Sink for the STe DMA stream. Applies the LMC1992 mix of the mono YM2149 output and the stereo
  //! DMA stream, using the coefficients programmed into the pluggable LMC1992 device.
  class Renderer
    : public Module::Renderer
    , public SteDma::Sink
  {
  public:
    Renderer(ModuleData::Ptr tune, Devices::AYM::Chip::Ptr chip, uint_t samplerate)
      : Tune(std::move(tune))
      , SampleRate(samplerate)
      , State(Tune->Duration)
      , Engine(std::move(chip), samplerate)
    {
      Engine.Dma().SetSink(this);
      Restart();
    }

    Module::State GetState() const override
    {
      return State.Get();
    }

    Sound::Chunk Render() override
    {
      const auto consumed = State.ConsumeUpTo(FRAME_DURATION);
      // Anchor the chunk to an absolute output grid: a timer handler may overrun the boundary by
      // its CPU time, and the next chunk must absorb that overrun instead of shifting the grid
      const auto target = NextRenderTime + consumed;
      AdvanceTo(target);
      NextRenderTime = target;
      // Render the chip up to the output grid, not to the engine time: a timer handler may have
      // overrun the boundary, and re-rendering an already produced time would underflow
      auto result = Engine.Ay().RenderFrame(target);
      MixDac(result);
      return result;
    }

    void Reset() override
    {
      State.Reset();
      Restart();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      // Reinitializing is the only reliable way to seek a native driver
      State.Reset();
      Restart();
      const auto target = Time::AtMicrosecond(request.CastTo<Time::Microsecond>());
      // Do not produce any audio while fast-forwarding
      Engine.Ay().SetBlocked(true);
      Blocked = true;
      AdvanceTo(target);
      Engine.Ay().SetBlocked(false);
      Blocked = false;
      DacFrames.clear();
      if (target.Get())
      {
        // Bring the chip's timeline up to the requested position, discarding the skipped part. Do it
        // in frame steps to keep the temporary buffer bounded.
        for (auto stamp = Time::AtMicrosecond() + FRAME_DURATION; stamp < target; stamp += FRAME_DURATION)
        {
          Engine.Ay().RenderFrame(stamp);
        }
        Engine.Ay().RenderFrame(target);
      }
      State.Seek(target);
      NextRenderTime = target;
    }

  private:
    // SteDma::Sink
    void AddDmaSample(int16_t left, int16_t right) override
    {
      if (Blocked)
      {
        return;
      }
      const auto coefs = Engine.Lmc().GetCoefficients();
      DacFrames.push_back(DacFrame{left, right, int16_t(coefs.Ym.Left().Raw()), int16_t(coefs.Dac.Left().Raw()),
                                   int16_t(coefs.Ym.Right().Raw()), int16_t(coefs.Dac.Right().Raw())});
    }

    //! Advance the machine up to the given output time, clocking the DMA between MFP events
    void AdvanceTo(Time::AtMicrosecond target)
    {
      while (Engine.GetTime() < target)
      {
        const auto stop = NextStop(target);
        Engine.ClockDac(stop);
        // ClockDac may already have advanced the machine time by running DMA end-of-buffer handlers
        const auto now = Engine.GetTime();
        if (now < stop)
        {
          Engine.Advance(stop - now);
        }
      }
    }

    void Restart()
    {
      Engine.Ay().Reset();
      Engine.Dma().Reset();
      Engine.Reset();
      Engine.Load(Tune->RAM);
      // Install the play routine as the MFP timer C interrupt handler and prime the AY registers
      Engine.Init(Tune->Index, Tune->Frequency);
      NextRenderTime = Time::AtMicrosecond();
    }

    //! Next point to advance to: the closest scheduled MFP event, but no further than target
    Time::AtMicrosecond NextStop(Time::AtMicrosecond target) const
    {
      const auto now = Engine.GetTime();
      const auto remaining = target - now;
      const auto toEvent = Engine.TimeToNextEvent();
      return toEvent.Get() < remaining.Get() ? now + toEvent : target;
    }

    //! Combine the YM2149 output with the stereo STe DMA stream through the LMC1992 coefficients
    void MixDac(Sound::Chunk& result)
    {
      const auto count = std::min(result.size(), DacFrames.size());
      for (std::size_t idx = 0; idx != count; ++idx)
      {
        const auto& frame = DacFrames[idx];
        // Keep the AY stereo image: mono is the default layout for SNDH, but the user may override
        // it, so the left and right channels must be mixed independently.
        const auto ymLeft = int32_t(result[idx].Left());
        const auto ymRight = int32_t(result[idx].Right());
        const auto left = (ymLeft * frame.YmLeft + int32_t(frame.Left) * frame.DacLeft) >> 8;
        const auto right = (ymRight * frame.YmRight + int32_t(frame.Right) * frame.DacRight) >> 8;
        result[idx] = Sound::Sample(Clamp(left), Clamp(right));
      }
      // A handler overrun may have produced DMA frames past this chunk. Keep them for the next one so
      // the DMA stays on the same output grid as the AY
      DacFrames.erase(DacFrames.begin(), DacFrames.begin() + count);
    }

    static Sound::Sample::Type Clamp(Sound::Sample::WideType value)
    {
      return static_cast<Sound::Sample::Type>(
          std::clamp<Sound::Sample::WideType>(value, Sound::Sample::MIN, Sound::Sample::MAX));
    }

  private:
    //! One host output sample of the STe DMA stream with the LMC1992 coefficients active at that time
    struct DacFrame
    {
      int16_t Left;
      int16_t Right;
      int16_t YmLeft;
      int16_t DacLeft;
      int16_t YmRight;
      int16_t DacRight;
    };

  private:
    const ModuleData::Ptr Tune;
    const uint_t SampleRate;
    TimedState State;
    Machine Engine;
    std::vector<DacFrame> DacFrames;
    // Absolute output grid the render chunks are anchored to (absorbs timer handler overruns)
    Time::AtMicrosecond NextRenderTime = Time::AtMicrosecond();
    bool Blocked = false;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(ModuleData::Ptr tune, Parameters::Accessor::Ptr properties)
      : Tune(std::move(tune))
      , Properties(std::move(properties))
    {}

    Module::Information GetModuleInformation() const override
    {
      return CreateTimedInfo(Tune->Duration.CastTo<Time::Millisecond>());
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto chip = AYM::CreateChip(samplerate, std::move(params));
      return MakePtr<Renderer>(Tune, std::move(chip), samplerate);
    }

  private:
    const ModuleData::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  class DataBuilder : public Formats::Multitrack::SNDH::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Module(MakeRWPtr<ModuleData>())
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetYear(StringView year) override
    {
      Properties.SetDate(year);
    }

    void SetUnknownTag(StringView /*tag*/, StringView /*value*/) override {}

    void SetTimer(char /*type*/, uint_t freq) override
    {
      // Guard against malformed tags: too high a rate would make sub-microsecond ticks
      const uint_t MAX_FREQUENCY = 1000000;
      Module->Frequency = freq >= 1 && freq <= MAX_FREQUENCY ? freq : DEFAULT_FREQUENCY;
    }

    void SetSubtunes(std::vector<StringView> names) override
    {
      Names = std::move(names);
    }

    void SetDurations(std::vector<Time::Seconds> durations) override
    {
      Durations = std::move(durations);
    }

    void SetFrames(std::vector<uint_t> frames) override
    {
      Frames = std::move(frames);
    }

    void SetFlags(StringView /*flags*/) override {}

    ModuleData::RWPtr CaptureResult(uint_t index)
    {
      Module->Index = index + 1;  // SNDH subsongs are 1-based
      if (index < Frames.size() && Frames[index])
      {
        Module->Duration = Time::Microseconds::FromRatio(Frames[index], Module->Frequency);
      }
      else if (index < Durations.size())
      {
        Module->Duration = Durations[index];
      }
      if (index < Names.size())
      {
        Properties.SetTitle(Names[index]);
      }
      return Module;
    }

  private:
    AYM::PropertiesHelper& Properties;
    MetaProperties Meta;
    ModuleData::RWPtr Module;
    std::vector<StringView> Names;
    std::vector<Time::Seconds> Durations;
    std::vector<uint_t> Frames;
  };

  Binary::Dump Copy(Binary::View data)
  {
    return {data.As<uint8_t>(), data.As<uint8_t>() + data.Size()};
  }

  class Factory : public MultitrackFactory
  {
  public:
    Holder::Ptr CreateModule(const Parameters::Accessor& params, const Formats::Multitrack::Container& data,
                             Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      props.SetPlatform(Platforms::ATARI);
      props.SetChipFrequency(YM_CLOCK);
      props.SetChipType(Devices::AYM::TYPE_YM2149F);
      // The STe wires the three YM2149 outputs to a single mono node feeding the LMC1992
      props.SetChannelsLayout(Devices::AYM::LAYOUT_MONO);
      props.SetSource(data);
      DataBuilder builder(props);
      Require(Formats::Multitrack::SNDH::Parse(data, builder) != nullptr);
      auto tune = builder.CaptureResult(data.StartTrackIndex());
      if (!tune->Duration)
      {
        tune->Duration = GetDefaultDuration(params);
      }
      tune->RAM = Copy(data);
      return MakePtr<Holder>(std::move(tune), std::move(properties));
    }
  };

  MultitrackFactory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::SNDH
