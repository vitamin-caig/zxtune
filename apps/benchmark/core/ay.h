/*
Abstract:
  AY tests interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BENCHMARK_AY_H_DEFINED
#define BENCHMARK_AY_H_DEFINED

//library includes
#include <devices/aym/chip.h>
#include <time/stamp.h>

namespace Benchmark
{
  namespace AY
  {
    Devices::AYM::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t soundFreq, Devices::AYM::InterpolationType interpolate);
    double Test(Devices::AYM::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration);
  }
}

#endif //BENCHMARK_AY_H_DEFINED
