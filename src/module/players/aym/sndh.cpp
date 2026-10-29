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
#include "module/players/aym/atari/timedb.h"
#include "module/players/aym/ay_data_channel.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_parameters.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/duration.h"
#include "module/players/platforms.h"
#include "module/players/properties_meta.h"
#include "module/players/streaming.h"

#include "binary/dump.h"
#include "binary/view.h"
#include "core/parameters.h"
#include "debug/log.h"
#include "devices/m68k.h"
#include "formats/multitrack/sndh.h"
#include "math/numeric.h"
#include "parameters/tracking_helper.h"
#include "sound/gain.h"
#include "sound/sample.h"
#include "time/oscillator.h"

#include "contract.h"
#include "make_ptr.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace Module::SNDH
{
  const Debug::Stream Dbg("Core::SNDHSupp");

  const uint_t DEFAULT_FREQUENCY = 50;

  //! The stereo STe DMA stream is exposed as one channel after the five YM2149 voices
  const Strings::Array DMA_CHANNELS{"DMA"};
  const uint_t DMA_MASK = 1 << Devices::AYM::VOICES;

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
  // and returns from the interrupt. The play routine is a plain subroutine for the drivers and may
  // run its own timer interrupts (e.g. a CPU-fed DAC needs the timer A handler to keep copying the
  // samples), so it runs at the reference player's VBL level: the level 6 MFP sources nest into it,
  // while the play timer itself stays in service until the explicit end-of-interrupt below, so it
  // cannot be re-entered even if it overruns its period (e.g. the Music Studio engine keeps its
  // state in the tick slot).
  const uint32_t PLAY_STUB_ADDR = 0x480;
  const uint8_t PLAY_STUB[] = {
      0x46, 0xfc, 0x24, 0x00,              // move.w #$2400,sr (allow the MFP sources to nest)
      0x42, 0x80,                          // clr.l d0
      0x4e, 0xb9, 0x00, 0x01, 0x00, 0x0a,  // jsr $0001000a
      0x08, 0xb8, 0x00, 0x05, 0xfa, 0x11,  // bclr.b #$5,$fffa11 (timer C end of interrupt)
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

  //! MFP 68901 timer block. Timers are clocked by the output timeline (one host sample at a time
  //! in the reference implementation), not by CPU cycles, because the CPU only runs in bursts.
  class Mfp : public Devices::M68K::Device
  {
  public:
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

    //! Queue an external event. The STe DMA end-of-frame is wired to timer A and GPI7 (as in
    //! AtariAudio Mk68901::SetSteDacExternalEvent); an event stays pending until a timer in event
    //! mode consumes it.
    void SignalExternalEvent()
    {
      Timers[TIMER_A].SetExternalEvent();
      Timers[GPI7].SetExternalEvent();
    }

    //! Consume the queued external events by the event-mode timers, latching their fires
    void ProcessExternalEvents()
    {
      for (uint_t idx = 0; idx != TIMERS_COUNT; ++idx)
      {
        if (Timers[idx].AdvanceEvent() && Timers[idx].Mask)
        {
          Timers[idx].Pending = true;
        }
      }
    }

    //! Advance the counters up to the given machine time, latching the fires of the enabled and
    //! unmasked sources. Deriving the tick delta from the oscillator avoids the drift of an
    //! explicit microsecond conversion.
    void Synchronize(Time::AtMicrosecond now)
    {
      // GetTickAtTime wraps around for a non-positive delta, so only move strictly forward
      if (!(Clock.GetCurrentTime() < now))
      {
        return;
      }
      const auto delta = Clock.GetTickAtTime(now) - Clock.GetCurrentTick();
      Clock.AdvanceTick(delta);
      for (uint_t idx = 0; idx != TIMERS_COUNT; ++idx)
      {
        if (Timers[idx].Advance(delta) && Timers[idx].Mask)
        {
          Timers[idx].Pending = true;
        }
      }
    }

    //! The play timer is owned by the host, which always writes the end-of-interrupt, while the
    //! driver's own sources follow the global automatic/software EOI mode
    bool SoftwareEoi(uint_t idx) const
    {
      return TIMER_C == idx || 0 != (Regs[REG_VR] & 0x08);
    }

    //! True when a pending source outranks the currently serviced one, following the MFP internal
    //! priority: a higher-priority timer may nest into a running handler, an equal or lower one must
    //! wait for the end-of-interrupt.
    bool IrqPending() const
    {
      uint_t serviceRank = TIMERS_COUNT;
      for (uint_t rank = 0; rank != TIMERS_COUNT; ++rank)
      {
        if (Timers[INTERRUPT_PRIORITY[rank]].InService)
        {
          serviceRank = rank;
          break;
        }
      }
      for (uint_t rank = 0; rank != serviceRank; ++rank)
      {
        const auto& timer = Timers[INTERRUPT_PRIORITY[rank]];
        if (timer.Pending && timer.Enable && timer.Mask)
        {
          return true;
        }
      }
      return false;
    }

    //! Acknowledge the highest priority pending source: latch it in service until the end-of-interrupt
    //! and return its vector number
    uint32_t Acknowledge()
    {
      for (const auto idx : INTERRUPT_PRIORITY)
      {
        auto& timer = Timers[idx];
        if (timer.Pending && timer.Enable && timer.Mask)
        {
          timer.Pending = false;
          // The play timer is owned by the host, which always writes the end-of-interrupt, so it
          // stays in service until then even in the driver's auto-EOI mode: a play routine that
          // overruns its period (or is preempted by the driver's higher-rate timers) must not be
          // re-entered. The other sources follow the global EOI mode.
          timer.InService = SoftwareEoi(idx);
          return uint32_t(TIMER_VECTORS[idx] / sizeof(uint32_t));
        }
      }
      return Devices::M68K::NO_VECTOR;
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

    //! Move the MFP clock to the given time without advancing the timers. Used to drop the timer
    //! events that would have happened before playback started.
    void SetTimeBase(Time::AtMicrosecond now)
    {
      Clock.AdvanceTime(now);
    }

  private:
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
      case REG_IPRA:
        // Pending bits follow the IER layout: A=5, B=0, GPI7=7
        return uint8_t((Timers[TIMER_A].Pending ? 1 << 5 : 0) | (Timers[TIMER_B].Pending ? 1 << 0 : 0)
                       | (Timers[GPI7].Pending ? 1 << 7 : 0));
      case REG_IPRB:
        return uint8_t((Timers[TIMER_C].Pending ? 1 << 5 : 0) | (Timers[TIMER_D].Pending ? 1 << 4 : 0));
      case REG_ISRA:
        return uint8_t((Timers[TIMER_A].InService ? 1 << 5 : 0) | (Timers[TIMER_B].InService ? 1 << 0 : 0)
                       | (Timers[GPI7].InService ? 1 << 7 : 0));
      case REG_ISRB:
        return uint8_t((Timers[TIMER_C].InService ? 1 << 5 : 0) | (Timers[TIMER_D].InService ? 1 << 4 : 0));
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
      case REG_ISRA:
        // Software end-of-interrupt: a zero bit releases the source, a one leaves it in service
        Timers[TIMER_A].InService = Timers[TIMER_A].InService && 0 != (data & (1 << 5));
        Timers[TIMER_B].InService = Timers[TIMER_B].InService && 0 != (data & (1 << 0));
        Timers[GPI7].InService = Timers[GPI7].InService && 0 != (data & (1 << 7));
        break;
      case REG_ISRB:
        Timers[TIMER_C].InService = Timers[TIMER_C].InService && 0 != (data & (1 << 5));
        Timers[TIMER_D].InService = Timers[TIMER_D].InService && 0 != (data & (1 << 4));
        break;
      default:
        break;
      }
      Regs[reg] = data;
    }

  private:
    //! Register window within the 24-bit address space
    static const uint32_t WINDOW_LO = 0xfffa00;
    static const uint32_t WINDOW_HI = WINDOW_LO + 0x26;
    //! The MFP is clocked by its own 2.4576 MHz crystal, independent of the master clock (the
    //! value divides cleanly to the standard RS-232 baud rates of the MFP's USART)
    static constexpr uint32_t CLOCK = 2457600;
    //! Divisors applied to the MFP clock for timer control register values 0x0..0x7
    static constexpr uint32_t PRESCALE[] = {0, 4, 10, 16, 50, 64, 100, 200};
    //! Service order by channel number, highest priority first: GPI7(15), A(13), B(8), C(5), D(4)
    static constexpr Timer INTERRUPT_PRIORITY[TIMERS_COUNT] = {GPI7, TIMER_A, TIMER_B, TIMER_C, TIMER_D};

    //! Registers (byte offsets within WINDOW_LO)
    enum Register
    {
      REG_GPIP = 0x01,
      REG_IERA = 0x07,
      REG_IERB = 0x09,
      REG_IPRA = 0x0b,
      REG_IPRB = 0x0d,
      REG_ISRA = 0x0f,
      REG_ISRB = 0x11,
      REG_IMRA = 0x13,
      REG_IMRB = 0x15,
      REG_VR = 0x17,
      REG_TACR = 0x19,
      REG_TBCR = 0x1b,
      REG_TCDCR = 0x1d,
      REG_TADR = 0x1f,
      REG_TBDR = 0x21,
      REG_TCDR = 0x23,
      REG_TDDR = 0x25,
    };

    struct TimerState
    {
      uint8_t Control = 0;
      uint8_t Data = 0;
      uint8_t DataInit = 0;
      bool Enable = false;
      bool Mask = false;
      // External event queued by a device; consumed by event-mode timers
      bool ExternalEvent = false;
      // Interrupt pending/in-service bits (IPRx/ISRx); the pending bit coalesces repeated fires
      bool Pending = false;
      bool InService = false;
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

    TimerState Timers[TIMERS_COUNT];
    uint8_t Regs[256] = {};
    // MFP ticks are derived from the machine timeline through a dedicated oscillator
    Time::MicrosecOscillator Clock;
  };

  //! Video shifter stub: SNDH drivers query the resolution and the PAL/NTSC sync registers
  class Shifter : public Devices::M68K::Device
  {
  public:
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

  private:
    //! Register window
    static const uint32_t WINDOW_LO = 0xff8200;
    static const uint32_t WINDOW_HI = WINDOW_LO + 0x100;

    //! Registers (byte offsets within WINDOW_LO)
    enum Register
    {
      REG_SYNC = 0x0a,
      REG_RESOLUTION = 0x60,
    };
  };

  //! LMC1992 digitally controlled stereo audio processor, used in the Atari STe for volume and tone
  //! control. A pluggable bus Device serving the Microwire window [0xFF8922, 0xFF8926) that owns the
  //! programmed coefficients and mixes the mono YM2149 output with the stereo STe DMA stream.
  class Lmc1992 : public Devices::M68K::Device
  {
  public:
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

    //! Mix the mono YM2149 output with the stereo DMA stream using the current coefficients
    Sound::Sample MixSamples(Sound::Sample ay, Sound::Sample dac) const
    {
      UpdateCoefficients();
      const auto left = (Coefs.Ym.Left() * ay.Left() + Coefs.Dac.Left() * dac.Left()).Integer();
      const auto right = (Coefs.Ym.Right() * ay.Right() + Coefs.Dac.Right() * dac.Right()).Integer();
      return Sound::Sample(Clamp(left), Clamp(right));
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

    static Sound::Sample::Type Clamp(Sound::Gain::Type::ValueType value)
    {
      return Math::Clamp<Sound::Gain::Type::ValueType>(value, Sound::Sample::MIN, Sound::Sample::MAX);
    }

  private:
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
  constexpr int DAC_UNIT = 64;

  //! Causal linear interpolation between two consecutive DMA frames. The window is shifted only
  //! when the source pointer advances, so the output keeps interpolating while the DMA holds frame.
  //! When interpolation is disabled the latest frame is held instead.
  template<int UNIT>
  class Interpolator
  {
  public:
    explicit Interpolator(uint_t scale)
      : Scale(int32_t(scale))
    {}

    void Reset()
    {
      Prev = Cur = Sound::Sample();
      HasCur = false;
    }

    void SetEnabled(bool enabled)
    {
      Enabled = enabled;
    }

    //! Shift the window to a newly fetched source frame. Frames enter the window already scaled to
    //! the output domain. The first call after Reset primes the window from silence, so a (re)start
    //! with interpolation ramps in instead of jumping to the first frame.
    void Advance(Sound::Sample cur)
    {
      Prev = Cur;
      Cur = ScaleSample(cur);
      HasCur = true;
    }

    //! Whether Advance has established the current frame after Reset
    bool HasCurrent() const
    {
      return HasCur;
    }

    //! Interpolate from the previous frame to the current one at the fractional position
    Sound::Sample Fetch(uint_t fraction) const
    {
      if (!Enabled)
      {
        return Cur;
      }
      const auto pos = int32_t(fraction);
      return Interpolate(Prev, Cur, Scale - pos, pos, Scale);
    }

  private:
    static Sound::Sample ScaleSample(Sound::Sample sample)
    {
      return Sound::Sample(sample.Left() * UNIT, sample.Right() * UNIT);
    }

    static Sound::Sample Interpolate(Sound::Sample from, Sound::Sample to, int32_t fromWeight, int32_t toWeight,
                                     int32_t scale)
    {
      return Sound::Sample(Interpolate(from.Left(), to.Left(), fromWeight, toWeight, scale),
                           Interpolate(from.Right(), to.Right(), fromWeight, toWeight, scale));
    }

    static int Interpolate(int from, int to, int32_t fromWeight, int32_t toWeight, int32_t scale)
    {
      // The frames are already in the output domain; keep their resolution while rounding
      const int64_t sum = int64_t(from) * fromWeight + int64_t(to) * toWeight;
      const int64_t half = (sum < 0 ? -int64_t(scale) : int64_t(scale)) / 2;
      return int((sum + half) / scale);
    }

  private:
    const int32_t Scale;
    bool Enabled = false;
    Sound::Sample Prev;
    Sound::Sample Cur;
    bool HasCur = false;
  };

  //! STe DMA sound replay, mapped at 0xFF8900. A pluggable bus Device that renders the host-rate
  //! output stream into an internal buffer, drained by the renderer when it mixes in the YM2149
  //! output. Register writes take effect immediately: the machine renders the stream up to every
  //! event stop before running the handlers that write, so a write never lands in the middle of an
  //! already rendered interval. The end-of-buffer event is reported to the caller of Render,
  //! which dispatches the corresponding MFP timer handlers.
  class SteDma : public Devices::M68K::Device
  {
  public:
    SteDma(Devices::M68K::Chip& core, uint_t hostRate)
      : Core(core)
      , HostRate(hostRate)
      , Interp(hostRate)
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
      Level = Sound::Sample();
      Interp.Reset();
      LastMode = 0xff;
      Frames.clear();
      Consumed = 0;
      Clock.Reset();
      Clock.SetFrequency(HostRate, HostRate);
      FirstWrite.reset();
    }

    //! Switch between linear interpolation and holding the latest frame. Affects only the upcoming
    //! output samples; the replay position is unchanged.
    void SetInterpolation(bool enabled)
    {
      Interp.SetEnabled(enabled);
    }

    uint8_t ReadByte(uint32_t addr) override
    {
      return Read8(addr - WINDOW_LO);
    }

    uint16_t ReadWord(uint32_t addr) override
    {
      return Read16(addr - WINDOW_LO);
    }

    void WriteByte(uint32_t addr, uint8_t data, const Time::MicrosecOscillator& clock) override
    {
      const bool wasEnabled = 0 != (Regs[REG_CONTROL] & 1);
      Write8(addr - WINDOW_LO, data);
      if (!FirstWrite && !wasEnabled && (Regs[REG_CONTROL] & 1))
      {
        FirstWrite = clock.GetCurrentTime();
      }
    }

    void WriteWord(uint32_t addr, uint16_t data, const Time::MicrosecOscillator& clock) override
    {
      const bool wasEnabled = 0 != (Regs[REG_CONTROL] & 1);
      Write16(addr - WINDOW_LO, data);
      if (!FirstWrite && !wasEnabled && (Regs[REG_CONTROL] & 1))
      {
        FirstWrite = clock.GetCurrentTime();
      }
    }

    std::optional<Time::AtMicrosecond> FirstSoundWrite() const
    {
      return FirstWrite;
    }

    //! Number of host output samples to produce up to the given time
    uint_t SamplesTill(Time::AtMicrosecond till) const
    {
      return Clock.SamplesTill(till);
    }

    //! Move the output clock to the given time without producing samples. Used to drop the replay
    //! time that elapsed before playback started.
    void SetTimeBase(Time::AtMicrosecond till)
    {
      if (const auto count = Clock.SamplesTill(till))
      {
        Clock.CommitSamples(count);
      }
    }

    //! Render up to the given number of host samples into the internal buffer, stopping right
    //! after the sample the replay pointer wrapped in, if any. Returns the number of rendered
    //! samples and whether the end-of-buffer event was met. The caller dispatches the event and
    //! calls again for the rest, so the handlers see the register state the wrapped sample produced.
    std::pair<uint_t, bool> Render(uint_t count)
    {
      // Reuse the capacity occupied by the consumed prefix instead of growing the storage
      if (Consumed && Frames.capacity() == Frames.size())
      {
        const auto pending = Frames.size() - Consumed;
        std::move(Frames.begin() + Consumed, Frames.end(), Frames.begin());
        Frames.resize(pending);
        Consumed = 0;
      }
      uint_t done = 0;
      bool wrapped = false;
      while (done != count && !wrapped)
      {
        wrapped = Step();
        Frames.push_back(Level);
        ++done;
      }
      return {done, wrapped};
    }

    //! Commit the rendered samples to the output clock. Kept separate from Render so a render
    //! split by end-of-buffer events still commits once: each commit is an independent fixed
    //! point division, and splitting it would drift the sample phase
    void CommitRendered(uint_t count)
    {
      Clock.CommitSamples(count);
    }

    //! Consume up to maxSamples of the produced samples, in the same order as the AY output; the
    //! span is valid until the next Render/Consume call
    std::span<const Sound::Sample> Consume(std::size_t maxSamples = std::numeric_limits<std::size_t>::max())
    {
      const auto avail = Frames.size() - Consumed;
      const auto result = std::span(Frames).subspan(Consumed, std::min(maxSamples, avail));
      Consumed += result.size();
      return result;
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
          Interp.Reset();
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

    //! Advance the replay by one host sample, producing the stereo level at the 0 dB LMC
    //! reference. Returns true if the replay reached the buffer end during this sample.
    bool Step()
    {
      if (0 == (Regs[REG_CONTROL] & 1))
      {
        Level = Sound::Sample();
        return false;
      }
      const bool stereo = 0 == (Regs[REG_MODE] & 0x80);
      const uint_t frameSize = stereo ? 2 : 1;
      InnerClock += STE_DAC_RATES[Regs[REG_MODE] & 3];
      // After a (re)start the window is primed with the first frame; the previous frame stays silent.
      // Every later step is covered by the Advance calls that move the pointer.
      if (!Interp.HasCurrent())
      {
        Interp.Advance(FetchFrame(SamplePtr, stereo));
      }
      bool wrapped = false;
      while (InnerClock >= HostRate)
      {
        InnerClock -= HostRate;
        SamplePtr += frameSize;
        if (SamplePtr == SampleEndPtr)
        {
          wrapped = true;
          FetchSamplePtr();
          if (0 == (Regs[REG_CONTROL] & (1 << 1)))
          {
            // In no-loop mode the replay stops at the buffer end instead of restarting
            Regs[REG_CONTROL] &= 0xfe;
            Level = Sound::Sample();
            InnerClock = 0;
            return true;
          }
        }
        Interp.Advance(FetchFrame(SamplePtr, stereo));
      }
      // The mode sets the frame rate for both mono and stereo (stereo consumes two bytes per
      // frame), so the same resampling serves every mode. Interpolating from the previous frame to
      // the current one keeps the replay causal, so the buffer/loop boundary needs no look-ahead;
      // without interpolation the latest frame is held until the pointer advances.
      Level = Interp.Fetch(InnerClock);
      return wrapped;
    }

    Sound::Sample FetchFrame(uint32_t addr, bool stereo) const
    {
      const auto left = int(Fetch(addr));
      const auto right = stereo ? int(Fetch(addr + 1)) : left;
      return Sound::Sample(left, right);
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

    Devices::M68K::Chip& Core;
    const uint_t HostRate;
    Interpolator<DAC_UNIT> Interp;
    uint8_t Regs[256] = {};
    uint32_t SamplePtr = 0;
    uint32_t SampleStartPtr = 0;
    uint32_t SampleEndPtr = 0;
    uint32_t InnerClock = 0;
    Sound::Sample Level;
    uint8_t LastMode = 0xff;
    // Host-rate clock mirroring the AY chip clock so both produce the same sample count for the
    // same output stamps
    Devices::Details::ClockSource<Time::AtMicrosecond> Clock;
    std::vector<Sound::Sample> Frames;
    // Index of the first frame not yet consumed by the renderer
    std::size_t Consumed = 0;
    std::optional<Time::AtMicrosecond> FirstWrite;
  };

  //! STe blitter (BLiTTER) register block at 0xFF8A00: a RAM-to-RAM transfer engine with shift,
  //! end-masks, 16 logical ops and a halftone pattern. A pluggable bus device; it latches a register
  //! file and runs the transfer to completion when the control register's busy bit is written set,
  //! reading and writing machine RAM through the owner. Ported from AtariAudio (sndh-player)
  //! src/Blitter.cpp by Arnaud Carré aka Leonard/Oxygene (@leonard_coder).
  class Blitter : public Devices::M68K::Device
  {
  public:
    explicit Blitter(Devices::M68K::Chip& core)
      : Core(core)
    {
      core.AddDevice(WINDOW_LO, WINDOW_HI, *this);
      Reset();
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
    //! Return to the power-on state
    void Reset()
    {
      std::fill(std::begin(Regs), std::end(Regs), uint8_t(0));
    }

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

  private:
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
      Write(addr, data, clock);
    }

    void WriteWord(uint32_t addr, uint16_t data, const Time::MicrosecOscillator& clock) override
    {
      Write(addr, uint8_t(data >> 8), clock);
    }

  private:
    void Write(uint32_t addr, uint8_t data, const Time::MicrosecOscillator& clock)
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

  private:
    //! Register window
    static const uint32_t WINDOW_LO = 0xff8800;
    static const uint32_t WINDOW_HI = 0xff8900;

    //! Registers: address bit 1 selects the data port over the register select
    enum Register
    {
      REG_SELECT = 0x00,
      REG_DATA = 0x02,
    };
  };

  // The 68000 is clocked by the master clock divided by 4
  const uint_t CPU_CLOCK_DIVISOR = 4;
  // The YM2149 is clocked by the master clock divided by 16
  const uint_t YM_CLOCK_DIVISOR = 16;

  //! Parameters adapter answering the device-specific settings from the Atari master clock and
  //! the STe wiring. The module properties (including the per-item ones) take priority, while
  //! the global parameters are shadowed by the hardware facts like the pinned module properties
  //! used to
  class AtariParametersAdapter : public Parameters::Accessor
  {
  public:
    AtariParametersAdapter(Parameters::Accessor::Ptr moduleProps, Parameters::Accessor::Ptr delegate)
      : ModuleProps(std::move(moduleProps))
      , Delegate(std::move(delegate))
    {}

    uint_t Version() const override
    {
      return Delegate->Version();
    }

    std::optional<Parameters::IntType> FindInteger(Parameters::Identifier name) const override
    {
      if (IsHardwareFixed(name))
      {
        if (const auto val = ModuleProps->FindInteger(name))
        {
          return val;
        }
        if (name == Parameters::ZXTune::Core::AYM::Chip::Key)
        {
          return Parameters::IntType(Devices::AYM::TYPE_YM2149F);
        }
        if (name == Parameters::ZXTune::Core::AYM::Layout::Key)
        {
          // The STe wires the three YM2149 outputs to a single mono node feeding the LMC1992
          return Parameters::IntType(Devices::AYM::LAYOUT_MONO);
        }
        return Parameters::IntType(MasterClock() / YM_CLOCK_DIVISOR);
      }
      if (const auto val = Delegate->FindInteger(name))
      {
        return val;
      }
      if (name == Parameters::ZXTune::Core::M68K::Clockrate::Key)
      {
        return Parameters::IntType(MasterClock() / CPU_CLOCK_DIVISOR);
      }
      return std::nullopt;
    }

    std::optional<Parameters::StringType> FindString(Parameters::Identifier name) const override
    {
      return Delegate->FindString(name);
    }

    Binary::Data::Ptr FindData(Parameters::Identifier name) const override
    {
      return Delegate->FindData(name);
    }

    void Process(Parameters::Visitor& visitor) const override
    {
      Delegate->Process(visitor);
    }

  private:
    static bool IsHardwareFixed(Parameters::Identifier name)
    {
      return name == Parameters::ZXTune::Core::AYM::Chip::Key || name == Parameters::ZXTune::Core::AYM::Layout::Key
             || name == Parameters::ZXTune::Core::AYM::Clockrate::Key;
    }

    uint_t MasterClock() const
    {
      using Parameters::ZXTune::Core::Atari::Clockrate;
      const auto val = Parameters::Get<Clockrate>(*Delegate);
      return Parameters::Validate<Clockrate>(val) ? val : Clockrate::Default;
    }

  private:
    const Parameters::Accessor::Ptr ModuleProps;
    const Parameters::Accessor::Ptr Delegate;
  };

  //! 68000 clock parameters passed through to the core. The core polls them, so the clockrate
  //! parameter applies on the fly.
  class M68KParameters : public Devices::M68K::ChipParameters
  {
  public:
    explicit M68KParameters(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint_t ClockFreq() const override
    {
      using Parameters::ZXTune::Core::M68K::Clockrate;
      const auto val = Parameters::Get<Clockrate>(*Params);
      return Parameters::Validate<Clockrate>(val) ? val : Clockrate::Default;
    }

    static const Parameters::IdentifiersSet ALL;

  private:
    const Parameters::Accessor::Ptr Params;
  };

  const Parameters::IdentifiersSet M68KParameters::ALL{Parameters::ZXTune::Core::M68K::Clockrate::Key,
                                                       Parameters::ZXTune::Core::Atari::Clockrate::Key};

  //! Atari ST machine: the generic 68k core plus the Atari chips (MFP, shifter stub, sound chip,
  //! STe DMA, LMC1992, blitter), the SNDH exception/trap/vector setup and the MFP-timer-C playback
  //! model. Owns every device, which register themselves on the bus; the machine dispatches their
  //! external events.
  class Machine : public Devices::M68K::Environment
  {
  public:
    Machine(Devices::AYM::Chip::Ptr ay, uint_t dacRate, Devices::M68K::ChipParameters::Ptr m68kParams)
      : Core(Devices::M68K::CreateChip(*this, std::move(m68kParams), RAM_SIZE))
      , MfpState(*Core)
      , ShifterState(*Core)
      , AyState(*Core, std::move(ay))
      , DmaState(*Core, dacRate)
      , LmcState(*Core)
      , BlitState(*Core)
    {
      Reset();
    }

    void Reset()
    {
      Core->Reset();
      MfpState.Reset();
      UpdateIrq();
      PlaybackStarted = false;

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
    //! timer C handler firing at the given frequency and prime the sound registers. From the start
    //! of the playback the main code runs in Advance(), with the play routine called from the timer
    //! interrupt.
    void Init(uint_t subsong, uint32_t frequency)
    {
      Core->SetFaulted(false);
      Core->ExecuteSubroutine(UPLOAD_ADDR, subsong, INIT_CYCLES);
      if (Core->Faulted())
      {
        StartPlayback();
        return;
      }
      // Route the play routine through the MFP timer C interrupt (the default SNDH timer): the
      // handler calls the play entry point and returns from the interrupt
      Core->WriteBlock(PLAY_STUB_ADDR, PLAY_STUB);
      Core->WriteDword(TIMER_VECTORS[Mfp::TIMER_C], PLAY_STUB_ADDR);
      MfpState.SetTimerC(frequency);
      // Set the initial sound registers before the first interrupt fires
      Core->ExecuteSubroutine(UPLOAD_ADDR + 8, 0, CALL_CYCLES);
      StartPlayback();
    }

    //! The track timeline starts once the driver initialization is over: the init runs on the real
    //! CPU clock and may prime the sound registers (even start a tone) while doing so, but that is
    //! not part of the output. Settle the sound chips up to the init end and rebase the clocks to it,
    //! so the playback begins with the primed state instead of replaying the last init transients.
    void StartPlayback()
    {
      const auto start = GetTime();
      AyState.RenderTill(start);
      DmaState.SetTimeBase(start);
      MfpState.SetTimeBase(start);
      PlaybackStart = start;
      PlaybackStarted = true;
    }

    Time::AtMicrosecond GetTime() const
    {
      return Core->GetOscillator().GetCurrentTime();
    }

    Time::AtMicrosecond GetPlaybackStart() const
    {
      return PlaybackStart;
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

    //! Produce the DMA samples up to the given machine time, latching the interrupts of the
    //! end-of-buffer events met on the way. Re-entrant calls are ignored; the outer clocking loop
    //! continues afterwards.
    void ClockDac(Time::AtMicrosecond till)
    {
      if (DacBusy)
      {
        return;
      }
      DacBusy = true;
      const uint_t count = DmaState.SamplesTill(till);
      uint_t done = 0;
      do
      {
        const auto rendered = DmaState.Render(count - done);
        done += rendered.first;
        if (rendered.second)
        {
          DispatchDmaEvent();
        }
      } while (done != count);
      DmaState.CommitRendered(count);
      DacBusy = false;
    }

    //! Move the machine timeline to the given time, executing the CPU (main code and the interrupts
    //! it takes) and rendering the DMA stream on the way
    void Advance(Time::AtMicrosecond target)
    {
      // Nothing to render when the machine already ran ahead of the output grid
      if (!(GetTime() < target))
      {
        return;
      }
      while (!Core->Faulted() && GetTime() < target)
      {
        // Stop no further than the next timer event, so its handler sees the exact output state of
        // the fire time; the CPU resumes from the same point on the next step. Timers that fire
        // while a long handler runs are latched by the per-slice sync and served after the handler
        // returns, or preempt it when the running code allows that priority.
        const auto now = GetTime();
        auto step = target - now;
        if (const auto toEvent = MfpState.TimeToNextEvent(); toEvent < step)
        {
          step = toEvent;
        }
        Core->RunUntil(now + step);
        MfpState.Synchronize(GetTime());
        UpdateIrq();
      }
      // A faulted engine produces silence but must not stall the output timeline
      Core->AdvanceTime(target);
      ClockDac(GetTime());
    }

    // Devices::M68K::Environment
    void OnCycles(const Devices::M68K::Oscillator& clock) override
    {
      // The driver initialization runs before the playback timeline exists; StartPlayback rebases
      // the sound clocks to its end, so the observations before it carry no output state
      if (!PlaybackStarted)
      {
        return;
      }
      // Advance the peripherals with the executed code: the DMA stream renders the samples up to
      // the current CPU time, and the MFP latches the timer fires met on the way. The interrupt line
      // follows immediately, so a busy main loop is preempted instead of coalescing the ticks it
      // overruns into a single pending bit.
      const auto now = clock.GetCurrentTime();
      ClockDac(now);
      MfpState.Synchronize(now);
      UpdateIrq();
    }

    // Devices::M68K::Environment
    uint32_t GetInterruptVector(uint_t /*level*/) override
    {
      return MfpState.Acknowledge();
    }

  private:
    //! Raise or release the CPU interrupt line to follow the MFP pending sources. The MFP sits on
    //! the Atari interrupt priority level 6.
    void UpdateIrq()
    {
      Core->SetIrqLevel(MfpState.IrqPending() ? MFP_IRQ_LEVEL : 0);
    }

    //! Notify the machine about the DMA end-of-buffer event; the event-mode MFP sources latch their
    //! interrupts, which the CPU takes on the next execution slice
    void DispatchDmaEvent()
    {
      MfpState.SignalExternalEvent();
      if (!Core->Faulted())
      {
        MfpState.ProcessExternalEvents();
        UpdateIrq();
      }
    }

  private:
    static constexpr uint_t MFP_IRQ_LEVEL = 6;

    Devices::M68K::Chip::Ptr Core;
    Mfp MfpState;
    Shifter ShifterState;
    AtariAyDevice AyState;
    SteDma DmaState;
    Lmc1992 LmcState;
    Blitter BlitState;
    bool DacBusy = false;
    // CPU time the track timeline starts at (the end of the init)
    Time::AtMicrosecond PlaybackStart = Time::AtMicrosecond();
    bool PlaybackStarted = false;
  };

  //! Renders the module: advances the machine in frame steps and mixes the YM2149 output with the
  //! rendered STe DMA stream through the LMC1992 coefficients.
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModuleData::Ptr tune, Devices::AYM::Chip::Ptr chip, uint_t samplerate, Parameters::Accessor::Ptr params)
      : Tune(std::move(tune))
      , SampleRate(samplerate)
      , State(Tune->Duration)
      , Engine(std::move(chip), samplerate, MakePtr<M68KParameters>(params))
      , Params(std::move(params))
    {
      ApplyParameters();
      Restart();
    }

    Module::State GetState() const override
    {
      return State.Get();
    }

    Sound::Chunk Render() override
    {
      ApplyParameters();
      const auto consumed = State.ConsumeUpTo(FRAME_DURATION);
      // Anchor the chunk to an absolute output grid: a timer handler may overrun the boundary by
      // its CPU time, and the next chunk must absorb that overrun instead of shifting the grid
      const auto target = NextRenderTime + consumed;
      Engine.Advance(target);
      NextRenderTime = target;
      // Render the chip up to the output grid, not to the engine time: a timer handler may have
      // overrun the boundary, and re-rendering an already produced time would underflow
      auto result = Engine.Ay().RenderTill(target);
      MixDac(result);
      return result;
    }

    void Reset() override
    {
      State.Reset();
      Params.Reset();
      ApplyParameters();
      Restart();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      const auto relative = Time::AtMicrosecond(request.CastTo<Time::Microsecond>());
      auto target = Time::AtMicrosecond(RestartTime.Get() + relative.Get());
      if (target < NextRenderTime)
      {
        // Rewinding a native driver is only reliable by reinitializing it
        State.Reset();
        Restart();
        target = Time::AtMicrosecond(RestartTime.Get() + relative.Get());
      }
      SkipTo(target);
    }

  private:
    //! Advance the machine and the AY up to the target output time without producing output. The
    //! skipped part is replayed in bounded steps so the pending AY writes and the discarded render
    //! buffer stay bounded; the chip state at the target matches the one a normal playback pass
    //! reaches, unlike a reset-and-replay that would leave it at the initialization registers.
    void SkipTo(Time::AtMicrosecond target)
    {
      if (NextRenderTime < target)
      {
        // Discard the DMA stream while skipping; the machine still advances its replay state
        for (auto stamp = NextRenderTime + FRAME_DURATION; stamp < target; stamp += FRAME_DURATION)
        {
          Engine.Advance(stamp);
          Engine.Ay().RenderTill(stamp);
          Engine.Dma().Consume();
        }
        Engine.Advance(target);
        Engine.Ay().RenderTill(target);
        Engine.Dma().Consume();
      }
      // The module state is playback-relative; the machine timeline starts after the driver init
      State.Seek(Time::AtMicrosecond(target.Get() - RestartTime.Get()));
      NextRenderTime = target;
    }

    void Restart()
    {
      Engine.Ay().Reset();
      Engine.Dma().Reset();
      Engine.Reset();
      Engine.Load(Tune->RAM);
      // Install the play routine as the MFP timer C interrupt handler and prime the AY registers
      Engine.Init(Tune->Index, Tune->Frequency);
      // The track timeline starts at the end of the init, so the initialization (which runs on the
      // real CPU clock now) does not contribute to the output
      RestartTime = Engine.GetPlaybackStart();
      NextRenderTime = RestartTime;
    }

    //! The channel mask and the DMA interpolation are live playback parameters and may be changed
    //! while the module is running
    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        ChannelMask = Params.Get<Parameters::ZXTune::Core::ChannelsMask>();
        Engine.Dma().SetInterpolation(Params.Get<Parameters::ZXTune::Core::DAC::Interpolation>());
      }
    }

    //! Combine the YM2149 output with the stereo STe DMA stream through the LMC1992 coefficients
    void MixDac(Sound::Chunk& result)
    {
      // A handler overrun may have produced DMA frames past this chunk; maxSamples keeps them for
      // the next one so the DMA stays on the same output grid as the AY
      const auto frames = Engine.Dma().Consume(result.size());
      // Muting affects only the rendered DMA contribution; the replay state keeps advancing
      const bool muted = 0 != (ChannelMask & DMA_MASK);
      for (std::size_t idx = 0; idx != frames.size(); ++idx)
      {
        result[idx] = Engine.Lmc().MixSamples(result[idx], muted ? Sound::Sample() : frames[idx]);
      }
    }

  private:
    const ModuleData::Ptr Tune;
    const uint_t SampleRate;
    TimedState State;
    Machine Engine;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
    uint64_t ChannelMask = 0;
    // Machine time the track timeline starts at (the end of the init)
    Time::AtMicrosecond RestartTime = Time::AtMicrosecond();
    // Absolute output grid the render chunks are anchored to (absorbs timer handler overruns)
    Time::AtMicrosecond NextRenderTime = Time::AtMicrosecond();
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
      auto adapted = MakePtr<AtariParametersAdapter>(Properties, std::move(params));
      auto chip = AYM::CreateChip(samplerate, adapted);
      return MakePtr<Renderer>(Tune, std::move(chip), samplerate, std::move(adapted));
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

    void SetSubtunes(Strings::Array names) override
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

    ModuleData::RWPtr CaptureResult(Binary::View module, uint_t index)
    {
      Module->Index = index + 1;  // SNDH subsongs are 1-based
      if (index < Frames.size() && Frames[index])
      {
        Module->Duration = Time::Microseconds::FromRatio(Frames[index], Module->Frequency);
      }
      // timedb overrides the unreliable TIME tag, but not an explicit FRMS
      else if (const auto timedbFrames = Atari::GetSndhFrames(module, index))
      {
        Module->Duration = Time::Microseconds::FromRatio(*timedbFrames, Module->Frequency);
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
    Strings::Array Names;
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
      // The device clocks and the YM2149 wiring are answered by AtariParametersAdapter at renderer
      // creation, so they follow the master clockrate parameter
      props.SetExtraChannels(DMA_CHANNELS);
      props.SetSupportedParameters(Module::AYM::ChipParameters::ALL | M68KParameters::ALL
                                   | Parameters::ZXTune::Core::DAC::Interpolation::Key);
      props.SetSource(data);
      DataBuilder builder(props);
      Require(Formats::Multitrack::SNDH::Parse(data, builder) != nullptr);
      auto tune = builder.CaptureResult(Binary::View(data.Start(), data.Size()), data.StartTrackIndex());
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
