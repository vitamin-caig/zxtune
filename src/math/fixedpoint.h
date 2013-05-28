/**
*
* @file     fixedpoint.h
* @brief    Fixed point arithmetic
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef MATH_FIXEDPOINT_H_DEFINED
#define MATH_FIXEDPOINT_H_DEFINED

//library includes
#include <math/scale.h>

namespace Math
{
  template<class T, unsigned Precision>
  class FixedPoint
  {
  public:
    typedef T ValueType;
    static const unsigned PRECISION = Precision;

    FixedPoint()
      : Value()
    {
    }

    template<class P>
    explicit FixedPoint(P integer)
      : Value(integer * PRECISION)
    {
    }

    template<class N, class D>
    FixedPoint(N nominator, D denominator)
    {
      if (denominator != PRECISION)
      {
        const std::pair<N, D> opt = OptimizeRatio(nominator, denominator);
        Value = opt.first * PRECISION / opt.second;
      }
      else
      {
        Value = nominator;
      }
    }

    FixedPoint(const FixedPoint<T, Precision>& rh)
      : Value(rh.Value)
    {
    }

    T Integer() const
    {
      return Value / Precision;
    }

    T Fraction() const
    {
      return Value % Precision;
    }

    FixedPoint<T, Precision>& operator = (T rh)
    {
      Value = rh * Precision;
      return *this;
    }

    FixedPoint<T, Precision>& operator = (const FixedPoint<T, Precision>& rh)
    {
      Value = rh.Value;
      return *this;
    }

    template<class P>
    FixedPoint<T, Precision>& operator += (P rh)
    {
      Value += rh * Precision;
      return *this;
    }

    FixedPoint<T, Precision>& operator += (const FixedPoint<T, Precision>& rh)
    {
      Value += rh.Value;
      return *this;
    }

    template<class P>
    FixedPoint<T, Precision>& operator -= (P rh)
    {
      Value -= rh * Precision;
      return *this;
    }

    FixedPoint<T, Precision>& operator -= (const FixedPoint<T, Precision>& rh)
    {
      Value -= rh.Value;
      return *this;
    }

    template<class P>
    bool operator < (P rh) const
    {
      return Integer() < rh;
    }

    bool operator < (const FixedPoint<T, Precision>& rh) const
    {
      return Value < rh.Value;
    }

    bool operator > (const FixedPoint<T, Precision>& rh) const
    {
      return Value > rh.Value;
    }

    FixedPoint<T, Precision> operator + (const FixedPoint<T, Precision>& rh) const
    {
      FixedPoint<T, Precision> res(*this);
      return res += rh;
    }

    FixedPoint<T, Precision> operator - (const FixedPoint<T, Precision>& rh) const
    {
      FixedPoint<T, Precision> res(*this);
      return res -= rh;
    }

    FixedPoint<T, Precision> operator * (const FixedPoint<T, Precision>& rh) const
    {
      FixedPoint<T, Precision> res(*this);
      res.Value = res.Value * rh.Value / rh.PRECISION;
      return res;
    }

    template<class P>
    FixedPoint<T, Precision> operator * (P rh) const
    {
      FixedPoint<T, Precision> res(*this);
      res.Value *= rh;
      return res;
    }

    template<class P>
    FixedPoint<T, Precision> operator / (P rh) const
    {
      FixedPoint<T, Precision> res(*this);
      res.Value /= rh;
      return res;
    }

    FixedPoint<T, Precision> operator / (const FixedPoint<T, Precision>& rh) const
    {
      FixedPoint<T, Precision> res(*this);
      res.Value = res.Value * rh.PRECISION / rh.Value;
      return res;
    }

    bool operator == (const FixedPoint<T, Precision>& rh) const
    {
      return Value == rh.Value;
    }

    bool operator != (const FixedPoint<T, Precision>& rh) const
    {
      return Value != rh.Value;
    }
  private:
    T Value;
  };
}

#endif //MATH_FIXEDPOINT_H_DEFINED
