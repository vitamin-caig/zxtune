/**
*
* @file      sound/gain.h
* @brief     Typedef for gain
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_GAIN_H_DEFINED
#define SOUND_GAIN_H_DEFINED

//common includes
#include <math/fixedpoint.h>

namespace Sound
{
  struct Gain
  {
  public:
    typedef Math::FixedPoint<int_t, 256> Type;
    static const uint_t CHANNELS = 2;

    Gain()
      : LeftVal()
      , RightVal()
    {
    }

    Gain(Type l, Type r)
      : LeftVal(l)
      , RightVal(r)
    {
    }

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
      //TODO: use Math::Clamp
      return LeftVal >= MIN && LeftVal <= MAX && RightVal >= MIN && RightVal <= MAX;
    }
  private:
    Type LeftVal;
    Type RightVal;
  };
}

#endif //SOUND_GAIN_H_DEFINED
