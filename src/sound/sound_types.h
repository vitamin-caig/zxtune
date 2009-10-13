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

#include <boost/array.hpp>

namespace ZXTune
{
  namespace Sound
  {
    /// Stereo mode
    const unsigned OUTPUT_CHANNELS = 2;
    
    /// Sound chunk type (by default unsigned 16bit LE)
    typedef uint16_t Sample;
    typedef boost::array<Sample, OUTPUT_CHANNELS> MultiSample;
    
    /// Gain specification type
    typedef double Gain;
    typedef boost::array<Gain, OUTPUT_CHANNELS> MultiGain;
  }
}

#endif //__SOUND_TYPES_H_DEFINED__
