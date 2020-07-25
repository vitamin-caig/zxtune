/**
*
* @file
*
* @brief  Time base type
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <math/scale.h>
//std includes
#include <type_traits>

namespace Time
{
  template<class T, T Resolution>
  struct BaseUnit
  {
    using StorageType = T;
    static const T PER_SECOND = Resolution;
  };

  using Second = BaseUnit<uint32_t, 1>;
  using Millisecond = BaseUnit<uint32_t, 1000>;
  using Microsecond = BaseUnit<uint64_t, 1000000>;
  using Nanosecond = BaseUnit<uint64_t, 1000000000ull>;

  struct InstantTag {};
  struct DurationTag {};

  template<class Unit, class Tag>
  class Base
  {
  public:
    using ValueType = typename Unit::StorageType;
    static const ValueType PER_SECOND = Unit::PER_SECOND;

    Base() = default;

    explicit Base(ValueType value)
      : Value(value)
    {
    }

    //from the other types with the same tag and lesser or equal precision
    template<class OtherUnit, typename std::enable_if<PER_SECOND >= OtherUnit::PER_SECOND, const void*>::type p = nullptr>
    Base(Base<OtherUnit, Tag> rh)
      : Value(Scale<OtherUnit, Unit>(rh.Get()))
    {
    }

    // Allow any precision of output
    template<class OtherUnit>
    Base<OtherUnit, Tag> CastTo() const
    {
      return Base<OtherUnit, Tag>(Scale<Unit, OtherUnit>(Get()));
    }

    bool operator < (Base rh) const
    {
      return Value < rh.Value;
    }

    bool operator == (Base rh) const
    {
      return Value == rh.Value;
    }

    template<class OtherUnit, typename std::enable_if<PER_SECOND < OtherUnit::PER_SECOND, const void*>::type p = nullptr>
    bool operator < (Base<OtherUnit, Tag> rh) const
    {
      return Base<OtherUnit, Tag>(*this) < rh;
    }

    template<class OtherUnit, typename std::enable_if<PER_SECOND < OtherUnit::PER_SECOND, const void*>::type p = nullptr>
    bool operator == (Base<OtherUnit, Tag> rh) const
    {
      return rh == *this;
    }

    ValueType Get() const
    {
      return Value;
    }
  private:
    template<class Unit1, class Unit2>
    static typename Unit2::StorageType Scale(typename Unit1::StorageType raw)
    {
      using MidType = typename std::common_type<typename Unit1::StorageType, typename Unit2::StorageType>::type;
      const MidType val = Unit1::PER_SECOND == Unit2::PER_SECOND
        ? raw
        : Math::Scale(MidType(raw), MidType(Unit1::PER_SECOND), MidType(Unit2::PER_SECOND));
      return static_cast<typename Unit2::StorageType>(val);
    }
  protected:
    ValueType Value = 0;
  };
}
