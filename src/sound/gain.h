/**
 *
 * @file
 *
 * @brief  Typedef for gain
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <math/fixedpoint.h>

namespace Sound
{
  struct Gain
  {
  public:
    using Type = Math::FixedPoint<int_t, 256>;
    static const uint_t CHANNELS = 2;

    Gain()
      : LeftVal()
      , RightVal()
    {}

    Gain(Type l, Type r)
      : LeftVal(l)
      , RightVal(r)
    {}

    Type Left() const
    {
      return LeftVal;
    }

    Type Right() const
    {
      return RightVal;
    }

    bool IsNormalized() const
    {
      static const Type MIN(0, Type::PRECISION);
      static const Type MAX(Type::PRECISION, Type::PRECISION);
      // TODO: use Math::Clamp
      return LeftVal >= MIN && LeftVal <= MAX && RightVal >= MIN && RightVal <= MAX;
    }

    bool operator==(const Gain& rh) const
    {
      return LeftVal == rh.LeftVal && RightVal == rh.RightVal;
    }

  private:
    Type LeftVal;
    Type RightVal;
  };
}  // namespace Sound
