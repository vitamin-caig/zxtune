/**
 *
 * @file
 *
 * @brief  AY test implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "ay.h"
// common includes
#include <make_ptr.h>
// library includes
#include <sound/matrix_mixer.h>
#include <time/timer.h>

namespace
{
  class AYParameters : public Devices::AYM::ChipParameters
  {
  public:
    AYParameters(uint64_t clockFreq, uint_t soundFreq, Devices::AYM::InterpolationType interpolate)
      : Clock(clockFreq)
      , Sound(soundFreq)
      , Interpolate(interpolate)
    {}

    uint_t Version() const override
    {
      return 1;
    }

    uint64_t ClockFreq() const override
    {
      return Clock;
    }

    uint_t SoundFreq() const override
    {
      return Sound;
    }

    Devices::AYM::ChipType Type() const override
    {
      return Devices::AYM::TYPE_AY38910;
    }

    Devices::AYM::InterpolationType Interpolation() const override
    {
      return Interpolate;
    }

    uint_t DutyCycleValue() const override
    {
      return 50;
    }

    uint_t DutyCycleMask() const override
    {
      return 0;
    }

    Devices::AYM::LayoutType Layout() const override
    {
      return Devices::AYM::LAYOUT_ABC;
    }

  private:
    const uint64_t Clock;
    const uint_t Sound;
    const Devices::AYM::InterpolationType Interpolate;
  };

}  // namespace

namespace Benchmark::AY
{
  Devices::AYM::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t soundFreq,
                                       Devices::AYM::InterpolationType interpolate)
  {
    const Devices::AYM::ChipParameters::Ptr params = MakePtr<AYParameters>(clockFreq, soundFreq, interpolate);
    return Devices::AYM::CreateChip(params, Sound::ThreeChannelsMatrixMixer::Create());
  }

  double Test(Devices::AYM::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration)
  {
    using namespace Devices::AYM;
    const Time::Timer timer;
    DataChunk chunk;
    chunk.Data[Registers::MIXER] = Registers::MASK_TONEA | Registers::MASK_NOISEB | Registers::MASK_TONEC
                                   | Registers::MASK_NOISEC;
    chunk.Data[Registers::VOLA] = 0;
    chunk.Data[Registers::VOLB] = Registers::MASK_VOL;
    chunk.Data[Registers::VOLC] = Registers::MASK_ENV | Registers::MASK_VOL;
    dev.RenderData(chunk);
    const auto period = frameDuration.CastTo<TimeUnit>();
    const auto frames = duration.Divide<uint_t>(frameDuration);
    for (uint_t val = 0; val != frames; ++val)
    {
      const uint_t tonLo = (val & 0xff);
      const uint_t tonHi = (val & 0xf00) >> 8;
      const uint_t noise = (val & 0x1f000) >> 12;
      const uint_t envLo = tonLo;
      const uint_t envHi = (val & 0xff00) >> 8;
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
    const auto elapsed = timer.Elapsed<TimeUnit>();
    return double(chunk.TimeStamp.Get()) / elapsed.Get();
  }
}  // namespace Benchmark::AY
