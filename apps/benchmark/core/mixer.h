/**
 *
 * @file
 *
 * @brief  Mixer test interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "time/duration.h"

#include "types.h"

namespace Benchmark::Mixer
{
  double Test(uint_t channels, const Time::Milliseconds& duration, uint_t soundFreq);
}  // namespace Benchmark::Mixer
