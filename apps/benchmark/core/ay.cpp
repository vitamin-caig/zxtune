/**
* 
* @file
*
* @brief  AY test implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "ay.h"
#include "timer.h"
//library includes
#include <sound/matrix_mixer.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  class AYParameters : public Devices::AYM::ChipParameters
  {
  public:
    AYParameters(uint64_t clockFreq, uint_t soundFreq, Devices::AYM::InterpolationType interpolate)
      : Clock(clockFreq)
      , Sound(soundFreq)
      , Interpolate(interpolate)
    {
    }

    virtual uint_t Version() const
    {
      return 1;
    }

    virtual uint64_t ClockFreq() const
    {
      return Clock;
    }

    virtual uint_t SoundFreq() const
    {
      return Sound;
    }

    virtual Devices::AYM::ChipType Type() const
    {
      return Devices::AYM::TYPE_AY38910;
    }

    virtual Devices::AYM::InterpolationType Interpolation() const
    {
      return Interpolate;
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
    const Devices::AYM::InterpolationType Interpolate;
  };

}

namespace Benchmark
{
  namespace AY
  {
    Devices::AYM::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t soundFreq, Devices::AYM::InterpolationType interpolate)
    {
      const Devices::AYM::ChipParameters::Ptr params = boost::make_shared<AYParameters>(clockFreq, soundFreq, interpolate);
      return Devices::AYM::CreateChip(params, Sound::ThreeChannelsMatrixMixer::Create(), Sound::Receiver::CreateStub());
    }

    double Test(Devices::AYM::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration)
    {
      using namespace Devices::AYM;
      const Timer timer;
      DataChunk chunk;
      chunk.Data[Registers::MIXER] = Registers::MASK_TONEA | Registers::MASK_NOISEB | Registers::MASK_TONEC | Registers::MASK_NOISEC;
      chunk.Data[Registers::VOLA] = 0;
      chunk.Data[Registers::VOLB] = Registers::MASK_VOL;
      chunk.Data[Registers::VOLC] = Registers::MASK_ENV | Registers::MASK_VOL;
      dev.RenderData(chunk);
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
        chunk.Data[Registers::TONEA_L] = chunk.Data[Registers::TONEB_L] = chunk.Data[Registers::TONEC_L] = tonLo;
        chunk.Data[Registers::TONEA_H] = chunk.Data[Registers::TONEB_H] = chunk.Data[Registers::TONEC_H] = tonHi;
        chunk.Data[Registers::TONEN] = noise;
        chunk.Data[Registers::TONEE_L] = envLo;
        chunk.Data[Registers::TONEE_H] = envHi;
        chunk.Data[Registers::ENV] = envTp;
        chunk.TimeStamp += period;
        dev.RenderData(chunk);
      }
      const Stamp elapsed = timer.Elapsed<Stamp>();
      return double(chunk.TimeStamp.Get()) / elapsed.Get();
    }
  }
}
