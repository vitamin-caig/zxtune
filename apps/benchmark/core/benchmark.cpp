/**
 *
 * @file
 *
 * @brief  Benchmark library implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "benchmark.h"

#include "ay.h"
#include "mixer.h"
#include "z80.h"

#include "binary/dump.h"
#include "strings/format.h"

#include "contract.h"
#include "make_ptr.h"

namespace Benchmark
{
  const unsigned SOUND_FREQ = 44100;
  const Time::Milliseconds FRAME_DURATION(20);
  const Time::Milliseconds TEST_DURATION(1000000);

  namespace AY
  {
    class PerformanceTest : public Benchmark::PerformanceTest
    {
    public:
      explicit PerformanceTest(Devices::AYM::InterpolationType interpolate)
        : Interpolate(interpolate)
      {}

      std::string Category() const override
      {
        return "AY chip emulation";
      }

      std::string Name() const override
      {
        switch (Interpolate)
        {
        case Devices::AYM::INTERPOLATION_NONE:
          return "No interpolation";
        case Devices::AYM::INTERPOLATION_LQ:
          return "LQ interpolation";
        case Devices::AYM::INTERPOLATION_HQ:
          return "HQ interpolation";
        default:
          Require(false);
          return "Invalid interpolation";
        }
      }

      double Execute() const override
      {
        const Devices::AYM::Chip::Ptr dev = CreateDevice(1750000, SOUND_FREQ, Interpolate);
        return Test(*dev, TEST_DURATION, FRAME_DURATION);
      }

    private:
      const Devices::AYM::InterpolationType Interpolate;
    };

    void ForAllTests(TestsVisitor& visitor)
    {
      visitor.OnPerformanceTest(PerformanceTest(Devices::AYM::INTERPOLATION_NONE));
      visitor.OnPerformanceTest(PerformanceTest(Devices::AYM::INTERPOLATION_LQ));
      visitor.OnPerformanceTest(PerformanceTest(Devices::AYM::INTERPOLATION_HQ));
    }
  }  // namespace AY

  namespace Z80
  {
    class MemoryPerformanceTest : public Benchmark::PerformanceTest
    {
    public:
      std::string Category() const override
      {
        return "Z80 emulation";
      }

      std::string Name() const override
      {
        return "Memory access";
      }

      double Execute() const override
      {
        static const uint8_t Z80_TEST_MEM[] = {
            0xf3,              // di
            0x21, 0x00, 0x00,  // ld hl,0
            0x11, 0x00, 0x00,  // ld de,0
            // loop:
            0x7e,       // ld a,(hl)
            0x12,       // ld (de),a
            0x23,       // inc hl
            0x13,       // inc de
            0x18, 0xfa  // jr loop
        };
        Binary::Dump mem(Z80_TEST_MEM, std::end(Z80_TEST_MEM));
        mem.resize(65536);
        const Devices::Z80::Chip::Ptr dev = CreateDevice(UINT64_C(3500000), 24, mem, Devices::Z80::ChipIO::Ptr());
        return Test(*dev, TEST_DURATION, FRAME_DURATION);
      }
    };

    class IoPerformanceTest : public Benchmark::PerformanceTest
    {
    public:
      std::string Category() const override
      {
        return "Z80 emulation";
      }

      std::string Name() const override
      {
        return "I/O ports access";
      }

      double Execute() const override
      {
        static const uint8_t Z80_TEST_IO[] = {
            0xf3,              // di
            0x01, 0x00, 0x00,  // ld bc,0
            // loop:
            0xed, 0x78,  // in a,(c)
            0xd3, 0x00,  // out (0),a
            0x18, 0xfa   // jr loop
        };
        Binary::Dump mem(Z80_TEST_IO, std::end(Z80_TEST_IO));
        mem.resize(65536);
        const Devices::Z80::Chip::Ptr dev = CreateDevice(UINT64_C(3500000), 24, mem, MakePtr<Z80Ports>());
        return Test(*dev, TEST_DURATION, FRAME_DURATION);
      }

    private:
      class Z80Ports : public Devices::Z80::ChipIO
      {
      public:
        uint8_t Read(uint16_t addr) override
        {
          Require(addr == 0);
          return 0x00;
        }

        void Write(const Devices::Z80::Oscillator& stamp, uint16_t addr, uint8_t data) override
        {
          Require(addr == 0);
          Require(data == 0x00);
          Dummy = stamp.GetCurrentTime();
        }

      private:
        Devices::Z80::Stamp Dummy;
      };
    };

    void ForAllTests(TestsVisitor& visitor)
    {
      visitor.OnPerformanceTest(MemoryPerformanceTest());
      visitor.OnPerformanceTest(IoPerformanceTest());
    }
  }  // namespace Z80

  namespace Mixer
  {
    class PerformanceTest : public Benchmark::PerformanceTest
    {
    public:
      explicit PerformanceTest(uint_t channels)
        : Channels(channels)
      {}

      std::string Category() const override
      {
        return "Mixer";
      }

      std::string Name() const override
      {
        return Strings::Format("{}-channels", Channels);
      }

      double Execute() const override
      {
        return Test(Channels, TEST_DURATION, SOUND_FREQ);
      }

    private:
      const uint_t Channels;
    };

    void ForAllTests(TestsVisitor& visitor)
    {
      for (uint_t chan = 1; chan <= 4; ++chan)
      {
        visitor.OnPerformanceTest(PerformanceTest(chan));
      }
    }
  }  // namespace Mixer

  void ForAllTests(TestsVisitor& visitor)
  {
    AY::ForAllTests(visitor);
    Z80::ForAllTests(visitor);
    Mixer::ForAllTests(visitor);
  }
}  // namespace Benchmark
