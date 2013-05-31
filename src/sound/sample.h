/**
*
* @file      sound/sample.h
* @brief     Declaration of sound sample
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_SAMPLE_H_DEFINED
#define SOUND_SAMPLE_H_DEFINED

//common includes
#include <types.h>

namespace Sound
{
  struct Sample
  {
  public:
    static const uint_t CHANNELS = 2;
    static const uint_t BITS = 16;
    typedef int16_t Type;
    static const Type MIN = -32768;
    static const Type MID = 0;
    static const Type MAX = 32767;

    Sample()
      : Value()
    {
    }

    Sample(Type left, Type right)
      : Value((StorageType(static_cast<uint16_t>(right)) << SHIFT) | static_cast<uint16_t>(left))
    {
    }

    Type Left() const
    {
      return static_cast<Type>(Value);
    }

    Type Right() const
    {
      return static_cast<Type>(Value >> SHIFT);
    }
  private:
    typedef uint32_t StorageType;
    static const uint_t SHIFT = 8 * sizeof(StorageType) / 2;
    StorageType Value;
  };
}

#endif //SOUND_SAMPLE_H_DEFINED
