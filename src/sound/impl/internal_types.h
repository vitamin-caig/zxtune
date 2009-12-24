/*
Abstract:
  Internal sound types and definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __SOUND_IMPL_INTERNAL_TYPES_H_DEFINED__
#define __SOUND_IMPL_INTERNAL_TYPES_H_DEFINED__

#include <sound/sound_types.h>

namespace ZXTune
{
  namespace Sound
  {
    // fixed point procession-related

    /// Precision for fixed-point calculations (num(float) = num(int) / FIXED_POINT_PRECISION)
    const unsigned FIXED_POINT_PRECISION = 256;
        
    template<class T>
    inline T Gain2Fixed(Gain gain)
    {
      return static_cast<T>(gain * FIXED_POINT_PRECISION);
    }
    
    template<class T>
    inline boost::array<T, OUTPUT_CHANNELS> MultiGain2MultiFixed(const MultiGain& mg)
    {
      boost::array<T, OUTPUT_CHANNELS> res;
      std::transform(mg.begin(), mg.end(), res.begin(), Gain2Fixed<T>);
      return res;
    }
  }
}

#endif //__SOUND_IMPL_INTERNAL_TYPES_H_DEFINED__
