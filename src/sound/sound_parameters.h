/**
*
* @file      sound/sound_parameters.h
* @brief     Render parameters names
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SOUND_PARAMETERS_H_DEFINED__
#define __SOUND_PARAMETERS_H_DEFINED__

//common includes
#include <parameters.h>

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

      //! Default value- 20mS (50Hz)
      const IntType FRAMEDURATION_DEFAULT = 20000;
      const IntType FRAMEDURATION_MIN = 1000;
      const IntType FRAMEDURATION_MAX = 1000000;
      //! Parameter name
      extern const NameType FRAMEDURATION;

      //@{
      //! @name Looped playback

      //! Parameter name
      extern const NameType LOOPED;
      //@}
    }
  }
}

#endif //__SOUND_PARAMETERS_H_DEFINED__
