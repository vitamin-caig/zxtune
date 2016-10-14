/**
*
* @file
*
* @brief  Time stamp interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <math/scale.h>

namespace Time
{
  const uint_t MILLISECONDS_PER_SECOND = 1000;
  const uint_t MICROSECONDS_PER_SECOND = 1000000;
  const uint64_t NANOSECONDS_PER_SECOND = UINT64_C(1000000000);
  const uint_t SECONDS_PER_MINUTE = 60;
  const uint_t MINUTES_PER_HOUR = 60;

  template<class T, T Resolution>
  class Stamp
  {
  public:
    typedef T ValueType;
    static const T PER_SECOND = Resolution;

    Stamp()
      : Value()
    {
    }

    explicit Stamp(T value)
      : Value(value)
    {
    }

    //from the same type
    Stamp(const Stamp& rh) = default;

    bool operator < (const Stamp& rh) const
    {
      return Value < rh.Value;
    }

    bool operator == (const Stamp& rh) const
    {
      return Value == rh.Value;
    }
    
    //from the other types
    template<class T1, T1 OtherResolution>
    Stamp(const Stamp<T1, OtherResolution>& rh)
      : Value(sizeof(T1) >= sizeof(T)
        ? static_cast<T>(Math::Scale(rh.Get(), OtherResolution, T1(Resolution)))
        : Math::Scale(T(rh.Get()), T(OtherResolution), Resolution))
    {
    }

    template<class T1, T1 OtherResolution>
    const Stamp<T, Resolution>& operator += (const Stamp<T1, OtherResolution>& rh)
    {
      Value += Stamp<T, Resolution>(rh).Get();
      static_assert(Resolution >= OtherResolution, "Invalid resolution");
      return *this;
    }

    template<class T1, T1 OtherResolution>
    Stamp<T, Resolution> operator + (const Stamp<T1, OtherResolution>& rh) const
    {
      static_assert(Resolution >= OtherResolution, "Invalid resolution");
      return Stamp<T, Resolution>(Value + Stamp<T, Resolution>(rh).Get());
    }

    template<class T1, T1 OtherResolution>
    bool operator < (const Stamp<T1, OtherResolution>& rh) const
    {
      return *this < Stamp<T, Resolution>(rh);
    }

    template<class T1, T1 OtherResolution>
    bool operator == (const Stamp<T1, OtherResolution>& rh) const
    {
      return *this == Stamp<T, Resolution>(rh);
    }

    T Get() const
    {
      return Value;
    }
  private:
    T Value;
  };

  typedef Stamp<uint32_t, 1> Seconds;
  typedef Stamp<uint32_t, MILLISECONDS_PER_SECOND> Milliseconds;
  typedef Stamp<uint64_t, MICROSECONDS_PER_SECOND> Microseconds;
  typedef Stamp<uint64_t, NANOSECONDS_PER_SECOND> Nanoseconds;

  template<class TimeStamp>
  typename TimeStamp::ValueType GetFrequencyForPeriod(const TimeStamp& ts)
  {
    return TimeStamp::PER_SECOND / ts.Get();
  }

  template<class TimeStamp>
  TimeStamp GetPeriodForFrequency(typename TimeStamp::ValueType freq)
  {
    return TimeStamp(TimeStamp::PER_SECOND / freq);
  }
}
