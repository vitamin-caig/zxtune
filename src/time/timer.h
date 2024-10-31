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

#include <ctime>
#include <type_traits>

namespace Time
{
  class Timer
  {
  public:
    using NativeUnit =
        BaseUnit<std::conditional<sizeof(std::clock_t) == sizeof(uint64_t), uint64_t, uint_t>::type, CLOCKS_PER_SEC>;

    Timer()
      : Start(std::clock())
    {}

    template<class Unit = NativeUnit>
    Duration<Unit> Elapsed() const
    {
      return Duration<NativeUnit>(std::clock() - Start).CastTo<Unit>();
    }

  private:
    // enable assignment
    std::clock_t Start;
  };
}  // namespace Time
