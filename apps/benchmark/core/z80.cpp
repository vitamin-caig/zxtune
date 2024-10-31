/**
 *
 * @file
 *
 * @brief  Z80 test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "z80.h"

#include "time/timer.h"

#include "make_ptr.h"

#include <utility>

namespace
{
  class Z80Parameters : public Devices::Z80::ChipParameters
  {
  public:
    Z80Parameters(uint64_t clockFreq, uint_t intTicks)
      : Clock(clockFreq)
      , Int(intTicks)
    {}

    uint_t Version() const override
    {
      return 1;
    }

    uint_t IntTicks() const override
    {
      return Int;
    }

    uint64_t ClockFreq() const override
    {
      return Clock;
    }

  private:
    const uint64_t Clock;
    const uint_t Int;
  };
}  // namespace

namespace Benchmark::Z80
{
  Devices::Z80::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t intTicks, Binary::View memory,
                                       Devices::Z80::ChipIO::Ptr io)
  {
    auto params = MakePtr<Z80Parameters>(clockFreq, intTicks);
    return Devices::Z80::CreateChip(std::move(params), memory, std::move(io));
  }

  double Test(Devices::Z80::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration)
  {
    using namespace Devices::Z80;
    const Time::Timer timer;
    const auto period = frameDuration.CastTo<TimeUnit>();
    const auto frames = duration.Divide<uint_t>(frameDuration);
    Stamp stamp;
    for (uint_t frame = 0; frame != frames; ++frame)
    {
      dev.Interrupt();
      dev.Execute(stamp += period);
    }
    const auto elapsed = timer.Elapsed();
    return (frameDuration * frames).Divide<double>(elapsed);
  }
}  // namespace Benchmark::Z80
