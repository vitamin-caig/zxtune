/*
Abstract:
  Z80 tests interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BENCHMARK_Z80_H_DEFINED
#define BENCHMARK_Z80_H_DEFINED

//library includes
#include <devices/z80.h>
#include <time/stamp.h>

namespace Benchmark
{
  namespace Z80
  {
    Devices::Z80::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t intTicks, const Dump& memory, Devices::Z80::ChipIO::Ptr io);
    double Test(Devices::Z80::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration);
  }
}

#endif //BENCHMARK_Z80_H_DEFINED
