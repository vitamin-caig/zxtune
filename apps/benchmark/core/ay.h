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
#include <devices/aym.h>
#include <time/stamp.h>

namespace Benchmark
{
  namespace AY
  {
    Devices::AYM::Device::Ptr CreateDevice(uint64_t clockFreq, uint_t soundFreq, bool interpolate);
    double Test(Devices::AYM::Device& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration);
  }
}

#endif //BENCHMARK_AY_H_DEFINED
