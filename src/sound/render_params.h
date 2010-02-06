/**
*
* @file      sound/render_params.h
* @brief     Rendering parameters definition. Used as POD-helper
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __RENDER_PARAMS_H_DEFINED__
#define __RENDER_PARAMS_H_DEFINED__

#include <sound/sound_parameters.h>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Looping mode
    enum LoopMode
    {
      //! Stop right after reaching the end
      LOOP_NONE = 0,
      //! Continue playback from internally defined position (beginning if not supported)
      LOOP_NORMAL,
      //! Continue playback from the beginning
      LOOP_BEGIN
    };

    //! @brief Input parameters for rendering
    struct RenderParameters
    {
      // Fill with the default parameters
      RenderParameters()
        : ClockFreq(Parameters::ZXTune::Sound::CLOCKRATE_DEFAULT)
        , SoundFreq(Parameters::ZXTune::Sound::FREQUENCY_DEFAULT)
        , FrameDurationMicrosec(Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT)
        , Looping(static_cast<LoopMode>(Parameters::ZXTune::Sound::LOOPMODE_DEFAULT))
      {
      }
    
      //! Basic clock frequency for PSG
      uint64_t ClockFreq;
      //! Rendering sound frequency
      uint_t SoundFreq;
      //! Frame duration in us
      uint_t FrameDurationMicrosec;
      //! Loop mode
      LoopMode Looping;

      //! Calculating PSG clocks count per one frame
      uint_t ClocksPerFrame() const
      {
        return static_cast<uint_t>(ClockFreq * FrameDurationMicrosec / 1000000);
      }

      //! Calculating sound samples count per one frame
      uint_t SamplesPerFrame() const
      {
        return static_cast<uint_t>(SoundFreq * FrameDurationMicrosec / 1000000);
      }
    };
  }
}

#endif //__RENDER_PARAMS_H_DEFINED__
