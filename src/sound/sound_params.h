/*
Abstract:
  Sound parameters definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __SOUND_PARAMS_H_DEFINED__
#define __SOUND_PARAMS_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Sound
  {
    /// Input parameters for rendering
    struct RenderParameters
    {
      /// Rendering sound frequency
      unsigned SoundFreq;
      /// Basic clock frequency (for PSG,CPU etc)
      unsigned ClockFreq;
      /// Frame duration in us
      unsigned FrameDurationMicrosec;
      /// Different flags
      uint32_t Flags;

      /// Helper functions
      unsigned ClocksPerFrame() const
      {
        return unsigned(uint64_t(ClockFreq) * FrameDurationMicrosec / 1e6);
      }

      unsigned SamplesPerFrame() const
      {
        return unsigned(uint64_t(SoundFreq) * FrameDurationMicrosec / 1e6);
      }
    };
  }
}

#endif //__SOUND_PARAMS_H_DEFINED__
