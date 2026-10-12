/**
 *
 * @file
 *
 * @brief  Players common parameters
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "parameters/accessor.h"
#include "parameters/tracking_helper.h"
#include "time/duration.h"

namespace Module
{
  class FrameDurationParameter
  {
  public:
    FrameDurationParameter(Parameters::Accessor::Ptr storage, Time::Microseconds def);

    FrameDurationParameter(const FrameDurationParameter&) = delete;
    FrameDurationParameter& operator=(const FrameDurationParameter&) = delete;
    FrameDurationParameter(FrameDurationParameter&&) = default;

    void Synchronize();

    template<class Unit = Time::Microsecond>
    Time::Duration<Unit> Get() const
    {
      return Value.CastTo<Unit>();
    }

  private:
    Parameters::TrackingHelper<Parameters::Accessor> Delegate;
    const Time::Microseconds Default;
    Time::Microseconds Value;
  };
}  // namespace Module
