/**
 *
 * @file
 *
 * @brief  Time instant type interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <time/base.h>

namespace Time
{
  // Specifies point in time. Only allowed to duration add/subtract
  template<class Unit>
  class Instant : public Base<Unit, InstantTag>
  {
    using Parent = Base<Unit, InstantTag>;
    using Parent::Value;

  public:
    using Parent::Parent;
    using Parent::Get;
    using Parent::PER_SECOND;
    using typename Parent::ValueType;

    Instant() = default;

    Instant(Parent rh)
      : Parent(std::move(rh))
    {}

    template<class OtherUnit>
    const Instant& operator+=(Base<OtherUnit, DurationTag> rh)
    {
      static_assert(PER_SECOND >= OtherUnit::PER_SECOND, "Invalid resolution");
      Value += Base<Unit, DurationTag>(rh).Get();
      return *this;
    }

    template<class OtherUnit>
    Instant operator+(Base<OtherUnit, DurationTag> rh) const
    {
      static_assert(PER_SECOND >= OtherUnit::PER_SECOND, "Invalid resolution");
      return Instant(Value + Base<Unit, DurationTag>(rh).Get());
    }
  };

  using AtMillisecond = Instant<Millisecond>;
  using AtMicrosecond = Instant<Microsecond>;
  using AtNanosecond = Instant<Nanosecond>;
}  // namespace Time
