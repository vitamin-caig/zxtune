/**
 *
 * @file
 *
 * @brief  Time duration interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <time/base.h>

namespace Time
{
  template<class Unit>
  class Duration : public Base<Unit, DurationTag>
  {
    using Parent = Base<Unit, DurationTag>;
    using Parent::Value;

  public:
    using Parent::Parent;
    using Parent::Get;
    using Parent::PER_SECOND;
    using typename Parent::ValueType;

    Duration() = default;

    constexpr Duration(Parent rh)
      : Parent(std::move(rh))
    {}

    template<class OtherUnit>
    constexpr const auto& operator+=(Duration<OtherUnit> rh)
    {
      Value += Duration(rh).Get();
      return *this;
    }

    template<class OtherUnit>
    constexpr auto operator+(Duration<OtherUnit> rh) const
    {
      using Return = std::conditional_t<PER_SECOND >= OtherUnit::PER_SECOND, Unit, OtherUnit>;
      return Duration<Return>(this->template CastTo<Return>().Get() + rh.template CastTo<Return>().Get());
    }

    template<class T>
    constexpr auto operator*(T mult) const
    {
      return Duration(static_cast<ValueType>(Value * mult));
    }

    // Do not use operator / to allow to specify return type
    template<class T, class OtherUnit>
    constexpr auto Divide(Duration<OtherUnit> rh) const
    {
      constexpr const auto gcd = std::gcd(PER_SECOND, OtherUnit::PER_SECOND);
      // (a/b) / (c/d) => (a*d)/(c*b) => (d/gcd)*a / (b/gcd)*c
      const auto divisor = (OtherUnit::PER_SECOND / gcd) * Get();
      const auto divider = (PER_SECOND / gcd) * rh.Get();
      return T(std::common_type_t<decltype(gcd), T>(divisor) / divider);
    }

    template<class T = ValueType>
    constexpr auto ToFrequency() const
    {
      return Get() ? (T(PER_SECOND) / Get()) : T();
    }

    constexpr explicit operator bool() const
    {
      return Get() != 0;
    }

    constexpr bool operator!() const
    {
      return Get() == 0;
    }

    template<class T>
    constexpr static auto FromFrequency(T frequency)
    {
      return Duration(T(PER_SECOND) / frequency);
    }

    constexpr static auto FromRatio(ValueType count, ValueType rate)
    {
      return rate ? Duration(Math::Scale(count, rate, PER_SECOND)) : Duration();
    }
  };

  using Seconds = Duration<Second>;
  using Milliseconds = Duration<Millisecond>;
  using Microseconds = Duration<Microsecond>;
  using Nanoseconds = Duration<Nanosecond>;
}  // namespace Time
