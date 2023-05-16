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

namespace Parameters::ZXTune::Sound::Mixer
{
  //! @brief Parameters#ZXTune#Sound#Mixer namespace prefix
  const auto PREFIX = Sound::PREFIX + "mixer"_id;

  //@{
  //! @brief Function to create parameter name
  Identifier LEVEL(uint_t totalChannels, uint_t inChannel, uint_t outChannel);

  //! @brief Function to get defaul percent-based level
  //! @see Parameters#ZXTune#Sound#GAIN_PRECISION
  IntType LEVEL_DEFAULT(uint_t totalChannels, uint_t inChannel, uint_t outChannel);
  //@}
}  // namespace Parameters::ZXTune::Sound::Mixer
