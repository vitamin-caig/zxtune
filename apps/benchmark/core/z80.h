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

// library includes
#include <devices/z80.h>
#include <time/duration.h>

namespace Benchmark::Z80
{
  Devices::Z80::Chip::Ptr CreateDevice(uint64_t clockFreq, uint_t intTicks, Binary::View memory,
                                       Devices::Z80::ChipIO::Ptr io);
  double Test(Devices::Z80::Chip& dev, const Time::Milliseconds& duration, const Time::Microseconds& frameDuration);
}  // namespace Benchmark::Z80
