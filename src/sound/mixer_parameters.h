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

// library includes
#include <sound/sound_parameters.h>

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
        const auto PREFIX = Sound::PREFIX + "mixer"_id;

        //@{
        //! @brief Function to create parameter name
        StringView LEVEL(uint_t totalChannels, uint_t inChannel, uint_t outChannel);

        //! @brief Function to get defaul percent-based level
        //! @see Parameters#ZXTune#Sound#GAIN_PRECISION
        IntType LEVEL_DEFAULT(uint_t totalChannels, uint_t inChannel, uint_t outChannel);
        //@}
      }  // namespace Mixer
    }    // namespace Sound
  }      // namespace ZXTune
}  // namespace Parameters
