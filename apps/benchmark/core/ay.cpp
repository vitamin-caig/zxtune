/*
Abstract:
  AY tests implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay.h"
#include "timer.h"
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  class AYParameters : public Devices::AYM::ChipParameters
  {
  public:
    AYParameters(uint64_t clockFreq, uint_t soundFreq, bool interpolate)
      : Clock(clockFreq)
      , Sound(soundFreq)
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

}

namespace Benchmark
{
  namespace AY
  {
    Devices::AYM::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t soundFreq, bool interpolate)
    {
      const Devices::AYM::ChipParameters::Ptr params = boost::make_shared<AYParameters>(clockFreq, soundFreq, interpolate);
      return Devices::AYM::CreateChip(params, Devices::AYM::Receiver::CreateStub());
    }

    double Test(Devices::AYM::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration)
    {
      using namespace Devices::AYM;
      const Timer timer;
      DataChunk chunk;
      chunk.Data[DataChunk::REG_MIXER] = DataChunk::REG_MASK_TONEA | DataChunk::REG_MASK_NOISEB | DataChunk::REG_MASK_TONEC | DataChunk::REG_MASK_NOISEC;
      chunk.Data[DataChunk::REG_VOLA] = 0;
      chunk.Data[DataChunk::REG_VOLB] = DataChunk::REG_MASK_VOL;
      chunk.Data[DataChunk::REG_VOLC] = DataChunk::REG_MASK_ENV | DataChunk::REG_MASK_VOL;
      chunk.Mask = (1 << DataChunk::REG_BEEPER) - 1;
      const Stamp period = frameDuration;
      const uint_t frames = Stamp(duration).Get() / period.Get();
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
      const Stamp elapsed = timer.Elapsed<Stamp>();
      return double(chunk.TimeStamp.Get()) / elapsed.Get();
    }
  }
}
