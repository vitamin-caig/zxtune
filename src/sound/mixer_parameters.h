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

#include "sound/parameters.h"

namespace Parameters::ZXTune::Sound::Mixer
{
  //! @brief Parameters#ZXTune#Sound#Mixer namespace prefix
  constexpr auto Prefix = Sound::Prefix + "mixer"_id;

  //! @brief Mixer level for a channels pair, percent-based
  //! @see Parameters#ZXTune#Sound#Gain#Precision
  struct Level
  {
    const Identifier Key = ""_id;
    const uint_t Default = 0;

    static Level For(uint_t channels, uint_t inChannel, uint_t outChannel);
  };
}  // namespace Parameters::ZXTune::Sound::Mixer
