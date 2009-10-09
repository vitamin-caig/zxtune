/*
Abstract:
  Sound-related types and definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __SOUND_TYPES_H_DEFINED__
#define __SOUND_TYPES_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Sound
  {
    /// Sound chunk type (by default unsigned 16bit LE)
    typedef uint16_t Sample;

    /// Stereo mode
    const unsigned OUTPUT_CHANNELS = 2;
    /// Precision for fixed-point calculations (num(float) = num(int) / FIXED_POINT_PRECISION)
    const unsigned FIXED_POINT_PRECISION = 256;

    /// Multisample types
    typedef Sample SampleArray[OUTPUT_CHANNELS];
    
    struct Multisample
    {
      SampleArray Data;
    };
  }
}

#endif //__SOUND_TYPES_H_DEFINED__
