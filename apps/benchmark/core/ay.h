/**
 *
 * @file
 *
 * @brief  AY test interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "devices/aym/chip.h"

#include "time/duration.h"

namespace Benchmark::AY
{
  Devices::AYM::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t soundFreq,
                                       Devices::AYM::InterpolationType interpolate);
  double Test(Devices::AYM::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration);
}  // namespace Benchmark::AY
