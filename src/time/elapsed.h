/**
 *
 * @file
 *
 * @brief  Elapsed time functor helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "time/duration.h"
#include "time/timer.h"

#include <optional>

namespace Time
{
  class Elapsed
  {
  public:
    using NativeUnit = Timer::NativeUnit;

    template<class DurationType>
    explicit Elapsed(const DurationType& period)
      : Period(period)
    {}

    bool operator()()
    {
      // the very first call is always due, otherwise the initial report would be
      // delayed by a full period
      if (Last && Last->Elapsed() < Period)
      {
        return false;
      }
      Last.emplace();
      return true;
    }

  private:
    const Duration<NativeUnit> Period;
    std::optional<Timer> Last;
  };
}  // namespace Time
