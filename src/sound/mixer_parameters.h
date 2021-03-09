/**
 *
 * @file
 *
 * @brief  Mixing parameters names
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
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
        //! @see Parameters#ZXTune#Sound#GAIN_PRECISION
        IntType LEVEL_DEFAULT(uint_t totalChannels, uint_t inChannel, uint_t outChannel);
        //@}
      }  // namespace Mixer
    }    // namespace Sound
  }      // namespace ZXTune
}  // namespace Parameters
