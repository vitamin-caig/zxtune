/**
 *
 * @file
 *
 * @brief  Fixed point arithmetic
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <math/scale.h>

namespace Math
{
  template<class T, T Precision>
  class FixedPoint
  {
  public:
    using ValueType = T;
    static const T PRECISION = Precision;

    FixedPoint()
      : Value()
    {}

    template<class P>
    explicit FixedPoint(P val)
      : Value(val * PRECISION)
    {}

    template<class N, class D>
    FixedPoint(N nominator, D denominator)
    {
      Set(nominator, denominator);
    }

    FixedPoint(const FixedPoint<T, Precision>& rh) = default;

    template<class T1, T1 Precision1>
    FixedPoint(const FixedPoint<T1, Precision1>& rh)
    {
      Set(rh.Raw(), Precision1);
    }

    T Round() const
    {
      return (Value + Precision / 2) / Precision;
    }

    T Integer() const
    {
      return Value / Precision;
    }

    T Fraction() const
    {
      return Value % Precision;
    }

    T Raw() const
    {
      return Value;
    }

    template<class P>
    FixedPoint<T, Precision>& operator=(P rh)
    {
      Value = static_cast<T>(rh * Precision);
      return *this;
    }

    FixedPoint<T, Precision>& operator=(const FixedPoint<T, Precision>& rh) = default;

    template<class T1, T1 Precision1>
    FixedPoint<T, Precision>& operator=(const FixedPoint<T1, Precision1>& rh)
    {
      Set(rh.Raw(), Precision1);
      return *this;
    }

    template<class P>
    FixedPoint<T, Precision>& operator+=(P rh)
    {
      Value += rh * Precision;
      return *this;
    }

    FixedPoint<T, Precision>& operator+=(const FixedPoint<T, Precision>& rh)
    {
      Value += rh.Value;
      return *this;
    }

    template<class P>
    FixedPoint<T, Precision>& operator-=(P rh)
    {
      Value -= rh * Precision;
      return *this;
    }

    FixedPoint<T, Precision>& operator-=(const FixedPoint<T, Precision>& rh)
    {
      Value -= rh.Value;
      return *this;
    }

    FixedPoint<T, Precision> operator+(const FixedPoint<T, Precision>& rh) const
    {
      FixedPoint<T, Precision> res(*this);
      return res += rh;
    }

    FixedPoint<T, Precision> operator-(const FixedPoint<T, Precision>& rh) const
    {
      FixedPoint<T, Precision> res(*this);
      return res -= rh;
    }

    FixedPoint<T, Precision> operator*(const FixedPoint<T, Precision>& rh) const
    {
      FixedPoint<T, Precision> res(*this);
      res.Value = res.Value * rh.Value / rh.PRECISION;
      return res;
    }

    template<class P>
    FixedPoint<T, Precision> operator*(P rh) const
    {
      FixedPoint<T, Precision> res(*this);
      res.Value *= rh;
      return res;
    }

    template<class P>
    FixedPoint<T, Precision> operator/(P rh) const
    {
      FixedPoint<T, Precision> res(*this);
      res.Value /= rh;
      return res;
    }

    FixedPoint<T, Precision> operator/(const FixedPoint<T, Precision>& rh) const
    {
      FixedPoint<T, Precision> res(*this);
      res.Value = res.Value * rh.PRECISION / rh.Value;
      return res;
    }

    FixedPoint<T, Precision> operator>>(int shift) const
    {
      FixedPoint<T, Precision> res(*this);
      res.Value >>= shift;
      return res;
    }

    bool operator<(const FixedPoint<T, Precision>& rh) const
    {
      return Value < rh.Value;
    }

    bool operator<=(const FixedPoint<T, Precision>& rh) const
    {
      return Value <= rh.Value;
    }

    bool operator>(const FixedPoint<T, Precision>& rh) const
    {
      return Value > rh.Value;
    }

    bool operator>=(const FixedPoint<T, Precision>& rh) const
    {
      return Value >= rh.Value;
    }

    bool operator==(const FixedPoint<T, Precision>& rh) const
    {
      return Value == rh.Value;
    }

    bool operator!=(const FixedPoint<T, Precision>& rh) const
    {
      return Value != rh.Value;
    }

  private:
    template<class N, class D>
    void Set(N nominator, D denominator)
    {
      if (denominator != PRECISION)
      {
        const std::pair<N, D> opt = OptimizeRatio(nominator, denominator);
        Value = static_cast<T>(opt.first * PRECISION / opt.second);
      }
      else
      {
        Value = static_cast<T>(nominator);
      }
    }

  private:
    T Value;
  };
}  // namespace Math
