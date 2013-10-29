/**
* 
* @file
*
* @brief  Z80 test
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "z80.h"
#include "timer.h"
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  class Z80Parameters : public Devices::Z80::ChipParameters
  {
  public:
    Z80Parameters(uint64_t clockFreq, uint_t intTicks)
      : Clock(clockFreq)
      , Int(intTicks)
    {
    }

    virtual uint_t Version() const
    {
      return 1;
    }

    virtual uint_t IntTicks() const
    {
      return Int;
    }

    virtual uint64_t ClockFreq() const
    {
      return Clock;
    }
  private:
    const uint64_t Clock;
    const uint_t Int;
  };
}

namespace Benchmark
{
  namespace Z80
  {
    Devices::Z80::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t intTicks, const Dump& memory, Devices::Z80::ChipIO::Ptr io)
    {
      const Devices::Z80::ChipParameters::Ptr params = boost::make_shared<Z80Parameters>(clockFreq, intTicks);
      return Devices::Z80::CreateChip(params, memory, io);
    }

    double Test(Devices::Z80::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration)
    {
      using namespace Devices::Z80;
      const Timer timer;
      const Stamp period = frameDuration;
      const uint_t frames = Stamp(duration).Get() / period.Get();
      Stamp stamp;
      for (uint_t frame = 0; frame != frames; ++frame)
      {
        dev.Interrupt();
        dev.Execute(stamp += period);
      }
      const Stamp elapsed = timer.Elapsed<Stamp>();
      return double(stamp.Get()) / elapsed.Get();
    }
  }
}
