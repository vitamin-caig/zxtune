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

//library includes
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

    Duration(Parent rh)
      : Parent(std::move(rh))
    {
    }

    template<class OtherUnit, class OtherTag>
    const Duration& operator += (Base<OtherUnit, OtherTag> rh)
    {
      Value += Duration(rh).Get();
      return *this;
    }

    template<class T>
    Duration operator * (T mult) const
    {
      return Duration(static_cast<ValueType>(Value * mult));
    }

    // Do not use operator / to allow to specify return type
    template<class T, class OtherUnit,
      class Common = typename std::conditional<Unit::PER_SECOND >= OtherUnit::PER_SECOND, Unit, OtherUnit>::type>
    T Divide(Duration<OtherUnit> rh) const
    {
      using CastType = typename std::common_type<T, typename Common::StorageType>::type;
      const CastType divisor = Duration<Common>(*this).Get();
      const CastType divider = Duration<Common>(rh).Get();
      return T(divisor / divider);
    }

    template<class T = ValueType>
    T ToFrequency() const
    {
      return Get() ? (T(PER_SECOND) / Get()) : T();
    }

    explicit operator bool () const
    {
      return Get() != 0;
    }

    bool operator ! () const
    {
      return Get() == 0;
    }

    template<class T>
    static Duration FromFrequency(T frequency)
    {
      return Duration(T(PER_SECOND) / frequency);
    }

    static Duration FromRatio(ValueType count, ValueType rate)
    {
      return rate
        ? Duration(Math::Scale(count, rate, PER_SECOND))
        : Duration();
    }
  };

  template<class Unit1, class Unit2, class Tag2,
    class Return = typename std::conditional<Unit1::PER_SECOND >= Unit2::PER_SECOND, Unit1, Unit2>::type>
  inline Duration<Return> operator + (Duration<Unit1> lh, Base<Unit2, Tag2> rh)
  {
    return Duration<Return>(lh) += rh;
  }

  using Seconds = Duration<Second>;
  using Milliseconds = Duration<Millisecond>;
  using Microseconds = Duration<Microsecond>;
  using Nanoseconds = Duration<Nanosecond>;
}
