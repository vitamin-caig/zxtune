/**
* 
* @file
*
* @brief  Z80 test interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/z80.h>
#include <time/duration.h>

namespace Benchmark
{
  namespace Z80
  {
    Devices::Z80::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t intTicks, const Dump& memory, Devices::Z80::ChipIO::Ptr io);
    double Test(Devices::Z80::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration);
  }
}
