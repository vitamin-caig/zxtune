/*
Abstract:
  Benchmark libray.implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "benchmark.h"
#include "ay.h"
#include "z80.h"
#include "mixer.h"
//common includes
#include <contract.h>
#include <tools.h>
//boost includes
#include <boost/format.hpp>
#include <boost/make_shared.hpp>

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
      explicit PerformanceTest(bool interpolate)
        : Interpolate(interpolate)
      {
      }

      virtual std::string Category() const
      {
        return "AY chip emulation";
      }

      virtual std::string Name() const
      {
        return Interpolate ? "With interpolation" : "Without interpolation";
      }

      virtual double Execute() const
      {
        const Devices::AYM::Device::Ptr dev = CreateDevice(1750000, SOUND_FREQ, Interpolate);
        return Test(*dev, TEST_DURATION, FRAME_DURATION);
      }
    private:
      const bool Interpolate;
    };

    void ForAllTests(TestsVisitor& visitor)
    {
      visitor.OnPerformanceTest(PerformanceTest(false));
      visitor.OnPerformanceTest(PerformanceTest(true));
    }
  }

  namespace Z80
  {
    class MemoryPerformanceTest : public Benchmark::PerformanceTest
    {
    public:
      virtual std::string Category() const
      {
        return "Z80 emulation";
      }

      virtual std::string Name() const
      {
        return "Memory access";
      }

      virtual double Execute() const
      {
        static const uint8_t Z80_TEST_MEM[] =
        {
          0xf3,             //di
          0x21, 0x00, 0x00, //ld hl,0
          0x11, 0x00, 0x00, //ld de,0
          //loop:
          0x7e,             //ld a,(hl)
          0x12,             //ld (de),a
          0x23,             //inc hl
          0x13,             //inc de
          0x18, 0xfa        //jr loop
        };
        Dump mem(Z80_TEST_MEM, ArrayEnd(Z80_TEST_MEM));
        mem.resize(65536);
        const Devices::Z80::Chip::Ptr dev = CreateDevice(UINT64_C(3500000), 24, mem, Devices::Z80::ChipIO::Ptr());
        return Test(*dev, TEST_DURATION, FRAME_DURATION);
      }
    };

    class IoPerformanceTest : public Benchmark::PerformanceTest
    {
    public:
      virtual std::string Category() const
      {
        return "Z80 emulation";
      }

      virtual std::string Name() const
      {
        return "I/O ports access";
      }

      virtual double Execute() const
      {
        static const uint8_t Z80_TEST_IO[] =
        {
          0xf3,             //di
          0x01, 0x00, 0x00, //ld bc,0
          //loop:
          0xed, 0x78,       //in a,(c)
          0xd3, 0x00,       //out (0),a
          0x18, 0xfa        //jr loop
        };
        Dump mem(Z80_TEST_IO, ArrayEnd(Z80_TEST_IO));
        mem.resize(65536);
        const Devices::Z80::Chip::Ptr dev = CreateDevice(UINT64_C(3500000), 24, mem, boost::make_shared<Z80Ports>());
        return Test(*dev, TEST_DURATION, FRAME_DURATION);
      }
    private:
      class Z80Ports : public Devices::Z80::ChipIO
      {
      public:
        virtual uint8_t Read(uint16_t addr)
        {
          Require(addr == 0);
          return 0x00;
        }

        virtual void Write(const Devices::Z80::Oscillator& stamp, uint16_t addr, uint8_t data)
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
  }

  namespace Mixer
  {
    class PerformanceTest : public Benchmark::PerformanceTest
    {
    public:
      explicit PerformanceTest(uint_t channels)
        : Channels(channels)
      {
      }

      virtual std::string Category() const
      {
        return "Mixer";
      }

      virtual std::string Name() const
      {
        return (boost::format("%u-channels") % Channels).str();
      }

      virtual double Execute() const
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
  }

  void ForAllTests(TestsVisitor& visitor)
  {
    AY::ForAllTests(visitor);
    Z80::ForAllTests(visitor);
    Mixer::ForAllTests(visitor);
  }
}
