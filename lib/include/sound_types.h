#ifndef __SOUND_TYPES_H_DEFINED__
#define __SOUND_TYPES_H_DEFINED__

#include <cstring>

namespace ZXTune
{
  namespace Sound
  {
    /// Sound chunk type (by default unsigned 16bit LE)
    typedef uint16_t Sample;
    /// Sound chunk type for intermediate operations
    typedef unsigned BigSample;

    /// Stereo mode
    const std::size_t OUTPUT_CHANNELS = 2;
    /// Precision for fixed-point calculations (num(float) = num(int) / FIXED_POINT_PRECISION)
    const uint16_t FIXED_POINT_PRECISION = 256;

    /// Arrays
    typedef BigSample BigSampleArray[OUTPUT_CHANNELS];
    typedef Sample SampleArray[OUTPUT_CHANNELS];

    /// Mixing point
    struct ChannelMixer
    {
      ChannelMixer() : Mute(false)
      {
      }
      /*explicit*/ChannelMixer(const SampleArray& ar) : Mute(false)
      {
        std::memcpy(Matrix, &ar[0], sizeof(Matrix));
      }
      bool Mute;
      SampleArray Matrix;
    };

    inline bool operator == (const ChannelMixer& lh, const ChannelMixer& rh)
    {
      return lh.Mute == rh.Mute && 0 == std::memcmp(lh.Matrix, rh.Matrix, sizeof(lh.Matrix));
    }
  }
}

#endif //__SOUND_TYPES_H_DEFINED__
