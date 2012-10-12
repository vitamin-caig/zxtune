/*
Abstract:
  ZXTune benchmark application

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <contract.h>
#include <tools.h>
#include <devices/aym.h>
#include <devices/z80.h>
#include <sound/mixer.h>
#include <ctime>
#include <iostream>
#include <boost/make_shared.hpp>

namespace
{
  class Timer
  {
  public:
    Timer()
      : Start(std::clock())
    {
    }

    template<class Stamp>
    Stamp Elapsed() const
    {
      const std::clock_t elapsed = std::clock() - Start;
      return Stamp(Stamp::PER_SECOND * elapsed / CLOCKS_PER_SEC);
    }
  private:
    const std::clock_t Start;
  };

  const unsigned SOUND_FREQ = 44100;
  const Time::Milliseconds FRAME_DURATION(20);
}

namespace
{
  class AYParameters : public Devices::AYM::ChipParameters
  {
  public:
    explicit AYParameters(bool interpolate)
      : Clock(1750000)
      , Sound(SOUND_FREQ)
      , Interpolation(interpolate)
    {
    }

    virtual uint64_t ClockFreq() const
    {
      return Clock;
    }

    virtual uint_t SoundFreq() const
    {
      return Sound;
    }

    virtual bool IsYM() const
    {
      return false;
    }

    virtual bool Interpolate() const
    {
      return Interpolation;
    }

    virtual uint_t DutyCycleValue() const
    {
      return 50;
    }

    virtual uint_t DutyCycleMask() const
    {
      return 0;
    }

    virtual Devices::AYM::LayoutType Layout() const
    {
      return Devices::AYM::LAYOUT_ABC;
    }
  private:
    const uint64_t Clock;
    const uint_t Sound;
    const bool Interpolation;
  };

  template<class Stamp>
  double Test(Devices::AYM::Device& dev, const Stamp& duration)
  {
    using namespace Devices::AYM;
    const Timer timer;
    DataChunk chunk;
    chunk.Data[DataChunk::REG_MIXER] = DataChunk::REG_MASK_TONEA | DataChunk::REG_MASK_NOISEB | DataChunk::REG_MASK_TONEC | DataChunk::REG_MASK_NOISEC;
    chunk.Data[DataChunk::REG_VOLA] = 0;
    chunk.Data[DataChunk::REG_VOLB] = DataChunk::REG_MASK_VOL;
    chunk.Data[DataChunk::REG_VOLC] = DataChunk::REG_MASK_ENV | DataChunk::REG_MASK_VOL;
    chunk.Mask = (1 << DataChunk::REG_BEEPER) - 1;
    const Time::Nanoseconds period = FRAME_DURATION;
    const uint_t frames = Time::Nanoseconds(duration).Get() / period.Get();
    for (uint_t val = 0; val != frames; ++val)
    {
      const uint_t tonLo = (val &    0xff);
      const uint_t tonHi = (val &   0xf00) >> 8;
      const uint_t noise = (val & 0x1f000) >> 12;
      const uint_t envLo = tonLo;
      const uint_t envHi = (val &  0xff00) >> 8;
      const uint_t envTp = (val & 0xf0000) >> 16;
      chunk.Data[DataChunk::REG_TONEA_L] = chunk.Data[DataChunk::REG_TONEB_L] = chunk.Data[DataChunk::REG_TONEC_L] = tonLo;
      chunk.Data[DataChunk::REG_TONEA_H] = chunk.Data[DataChunk::REG_TONEB_H] = chunk.Data[DataChunk::REG_TONEC_H] = tonHi;
      chunk.Data[DataChunk::REG_TONEN] = noise;
      chunk.Data[DataChunk::REG_TONEE_L] = envLo;
      chunk.Data[DataChunk::REG_TONEE_H] = envHi;
      chunk.Data[DataChunk::REG_ENV] = envTp;
      chunk.TimeStamp += period;
      dev.RenderData(chunk);
      dev.Flush();
    }
    const Time::Nanoseconds elapsed = timer.Elapsed<Time::Nanoseconds>();
    return double(chunk.TimeStamp.Get()) / elapsed.Get();
  }

  void TestAY()
  {
    std::cout << "Test for AY chip emulation\n";
    const Time::Milliseconds emulationTime(1000000);//1000s
    {
      const Devices::AYM::ChipParameters::Ptr params = boost::make_shared<AYParameters>(false);
      const Devices::AYM::Chip::Ptr chip = Devices::AYM::CreateChip(params, Devices::AYM::Receiver::CreateStub());
      std::cout << " without interpolation: x" << Test(*chip, emulationTime) << std::endl;
    }
    {
      const Devices::AYM::ChipParameters::Ptr params = boost::make_shared<AYParameters>(true);
      const Devices::AYM::Chip::Ptr chip = Devices::AYM::CreateChip(params, Devices::AYM::Receiver::CreateStub());
      std::cout << " with interpolation: x" << Test(*chip, emulationTime) << std::endl;
    }
  }
}

namespace
{
  class Z80Parameters : public Devices::Z80::ChipParameters
  {
  public:
    virtual uint_t IntTicks() const
    {
      return 24;
    }

    virtual uint64_t ClockFreq() const
    {
      return UINT64_C(3500000);
    }
  };

  class Z80Ports : public Devices::Z80::ChipIO
  {
  public:
    virtual uint8_t Read(uint16_t addr)
    {
      Require(addr == 0);
      return 0x00;
    }

    virtual void Write(const Time::NanosecOscillator& stamp, uint16_t addr, uint8_t data)
    {
      Require(addr == 0);
      Require(data == 0x00);
      Dummy = stamp.GetCurrentTime();
    }
  private:
    Time::Nanoseconds Dummy;
  };

  template<class Stamp>
  double Test(Devices::Z80::Chip& chip, const Stamp& duration)
  {
    using namespace Devices::Z80;
    const Timer timer;
    const Time::Nanoseconds period = FRAME_DURATION;
    const uint_t frames = Time::Nanoseconds(duration).Get() / period.Get();
    Time::Nanoseconds stamp;
    for (uint_t frame = 0; frame != frames; ++frame)
    {
      chip.Interrupt();
      chip.Execute(stamp += period);
    }
    const Time::Nanoseconds elapsed = timer.Elapsed<Time::Nanoseconds>();
    return double(stamp.Get()) / elapsed.Get();
  }

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

  static const uint8_t Z80_TEST_IO[] =
  {
    0xf3,             //di
    0x01, 0x00, 0x00, //ld bc,0
    //loop:
    0xed, 0x78,       //in a,(c)
    0xd3, 0x00,       //out (0),a
    0x18, 0xfa        //jr loop
  };

  void TestZ80()
  {
    std::cout << "Test for Z80 emulation\n";
    const Time::Milliseconds emulationTime(1000000);//1000s
    const Devices::Z80::ChipParameters::Ptr params = boost::make_shared<Z80Parameters>();
    {
      Dump mem(Z80_TEST_MEM, ArrayEnd(Z80_TEST_MEM));
      mem.resize(65536);
      const Devices::Z80::Chip::Ptr chip = Devices::Z80::CreateChip(params, mem, Devices::Z80::ChipIO::Ptr());
      std::cout << " memory access: x" << Test(*chip, emulationTime) << std::endl;
    }
    {
      Dump mem(Z80_TEST_IO, ArrayEnd(Z80_TEST_IO));
      mem.resize(65536);
      const Devices::Z80::Chip::Ptr chip = Devices::Z80::CreateChip(params, mem, boost::make_shared<Z80Ports>());
      std::cout << " i/o ports access: x" << Test(*chip, emulationTime) << std::endl;
    }
  }
}

namespace
{
  template<class Stamp>
  double Test(ZXTune::Sound::Mixer& mix, uint_t channels, const Stamp& period)
  {
    std::vector<ZXTune::Sound::Sample> input(channels);
    const Timer timer;
    const uint_t totalFrames = uint64_t(period.Get()) * SOUND_FREQ / period.PER_SECOND;
    for (uint_t frame = 0; frame != totalFrames; ++frame)
    {
      mix.ApplyData(input);
    }
    mix.Flush();
    const Time::Nanoseconds elapsed = timer.Elapsed<Time::Nanoseconds>();
    const Time::Nanoseconds emulated(period);
    return double(emulated.Get()) / elapsed.Get();
  }

  void TestMixer()
  {
    std::cout << "Test for mixer\n";
    const Time::Milliseconds emulationTime(1000000);//1000s
    for (uint_t chan = 1; chan <= 4; ++chan)
    {
      const ZXTune::Sound::Mixer::Ptr mixer = ZXTune::Sound::CreateMatrixMixer(chan);
      std::cout << ' ' << chan << "-channels: x" << Test(*mixer, chan, emulationTime) << std::endl;
    }
  }
}

int main()
{
  TestAY();
  TestZ80();
  TestMixer();
}
