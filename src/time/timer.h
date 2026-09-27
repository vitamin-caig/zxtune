/**
 *
 * @file
 *
 * @brief  Simple elapsed timer
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "time/duration.h"

#include <chrono>
#include <ratio>

namespace Time
{
  // monotonic wall time; std::clock() cost a syscall per call and callers want real elapsed time
  class Timer
  {
  public:
    using Clock = std::chrono::steady_clock;
    static_assert(std::ratio_equal<Clock::period, std::nano>::value, "Clock must tick in nanoseconds");
    using NativeUnit = Nanosecond;
    using NativeDuration = Duration<NativeUnit>;

    Timer()
      : Start(Clock::now())
    {}

    template<class Unit = NativeUnit>
    Duration<Unit> Elapsed() const
    {
      const auto elapsed = (Clock::now() - Start).count();
      return Duration<NativeUnit>(elapsed).CastTo<Unit>();
    }

  private:
    // enable assignment
    Clock::time_point Start;
  };
}  // namespace Time
