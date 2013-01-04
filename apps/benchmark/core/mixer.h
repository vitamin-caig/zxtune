/*
Abstract:
  Mixer tests interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BENCHMARK_MIXER_H_DEFINED
#define BENCHMARK_MIXER_H_DEFINED

//library includes
#include <time/stamp.h>

namespace Benchmark
{
  namespace Mixer
  {
    double Test(uint_t channels, const Time::Milliseconds& duration, uint_t soundFreq);
  }
}

#endif //BENCHMARK_MIXER_H_DEFINED
