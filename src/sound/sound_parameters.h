/**
*
* @file
*
* @brief  Render parameters names
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/types.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief %Sound parameters namespace
    namespace Sound
    {
      //! @brief Parameters#ZXTune#Sound namespace prefix
      extern const NameType PREFIX;

      //@{
      //! @name %Sound frequency in Hz

      //! Default value- 44.1kHz
      const IntType FREQUENCY_DEFAULT = 44100;
      //! Parameter name
      extern const NameType FREQUENCY;
      //@}

      //@{
      //! @name Frame duration in microseconds
      const IntType FRAMEDURATION_PRECISION = 1000000;

      //! Default value- 20mS (50Hz)
      const IntType FRAMEDURATION_DEFAULT = 20000;
      const IntType FRAMEDURATION_MIN = 1000;
      const IntType FRAMEDURATION_MAX = 1000000;
      //! Parameter name
      extern const NameType FRAMEDURATION;
      //@}

      //@{
      //! @name Looped playback

      //! Parameter name
      extern const NameType LOOPED;
      //@}

      //@{
      //! @name Fadein in microseconds
      const IntType FADEIN_PRECISION = 1000000;

      //! Default value- no fading
      const IntType FADEIN_DEFAULT = 0;
      //! Parameter name
      extern const NameType FADEIN;
      //@}

      //@{
      //! @name Fadeout in microseconds
      const IntType FADEOUT_PRECISION = 1000000;

      //! Default value- no fading
      const IntType FADEOUT_DEFAULT = 0;
      //! Parameter name
      extern const NameType FADEOUT;
      //@}
      
      //@{
      //! @name Gain in percents
      const IntType GAIN_PRECISION = 100;
      
      //! Default value - no gain
      const IntType GAIN_DEFAULT = GAIN_PRECISION;
      //! Parameter name
      extern const NameType GAIN;
    }
  }
}
