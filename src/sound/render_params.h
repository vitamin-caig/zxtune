/*
Abstract:
  Rendering parameters definition. Used as POD-helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __RENDER_PARAMS_H_DEFINED__
#define __RENDER_PARAMS_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Sound
  {
    /// Input parameters for rendering
    struct RenderParameters
    {
      /// Fill with the default parameters
      RenderParameters()
        : ClockFreq(17500000)//1.75MHz
	, SoundFreq(44100)//44.1kHz
	, FrameDurationMicrosec(20000)//50Hz
      {
      }
    
      /// Basic clock frequency (for PSG,CPU etc)
      uint64_t ClockFreq;
      /// Rendering sound frequency
      unsigned SoundFreq;
      /// Frame duration in us
      unsigned FrameDurationMicrosec;

      /// Helper functions
      unsigned ClocksPerFrame() const
      {
        return static_cast<unsigned>(ClockFreq * FrameDurationMicrosec / 1e6);
      }

      unsigned SamplesPerFrame() const
      {
        return static_cast<unsigned>(SoundFreq * FrameDurationMicrosec / 1e6);
      }
    };
  }
}

#endif //__RENDER_PARAMS_H_DEFINED__
