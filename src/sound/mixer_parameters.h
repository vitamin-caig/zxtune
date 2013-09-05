/**
*
* @file      sound/mixer_parameters.h
* @brief     Mixing parameters names
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef MIXER_PARAMETERS_H_DEFINED
#define MIXER_PARAMETERS_H_DEFINED

//common includes
#include <parameters/types.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief %Sound parameters namespace
    namespace Sound
    {
      //! @brief %Mixer parameters namespace
      namespace Mixer
      {
        //! @brief Parameters#ZXTune#Sound#Mixer namespace prefix
        extern const NameType PREFIX;

        //@{
        //! @brief Function to create parameter name
        NameType LEVEL(uint_t totalChannels, uint_t inChannel, uint_t outChannel);

        //! @brief Function to get defaul percent-based level
        IntType LEVEL_DEFAULT(uint_t totalChannels, uint_t inChannel, uint_t outChannel);
        //@}
      }
    }
  }
}

#endif //MIXER_PARAMETERS_H_DEFINED
