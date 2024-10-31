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

#include "zxtune.h"

#include "parameters/types.h"

namespace Parameters::ZXTune::Sound
{
  //! @brief Parameters#ZXTune#Sound namespace prefix
  const auto PREFIX = ZXTune::PREFIX + "sound"_id;

  //@{
  //! @name %Sound frequency in Hz

  //! Default value- 44.1kHz
  const IntType FREQUENCY_DEFAULT = 44100;
  //! Parameter name
  const auto FREQUENCY = PREFIX + "frequency"_id;
  //@}

  //@{
  //! @name Looped playback

  //! Parameter name
  const auto LOOPED = PREFIX + "looped"_id;
  const auto LOOP_LIMIT = PREFIX + "looplimit"_id;
  //@}

  //@{
  //! @name Fadein in seconds
  const IntType FADEIN_PRECISION = 1;

  //! Default value- no fading
  const IntType FADEIN_DEFAULT = 0;
  //! Parameter name
  const auto FADEIN = PREFIX + "fadein"_id;
  //@}

  //@{
  //! @name Fadeout in seconds
  const IntType FADEOUT_PRECISION = 1;

  //! Default value- no fading
  const IntType FADEOUT_DEFAULT = 0;
  //! Parameter name
  const auto FADEOUT = PREFIX + "fadeout"_id;
  //@}

  //@{
  //! @name Gain in percents
  const IntType GAIN_PRECISION = 100;

  //! Default value - no gain
  const IntType GAIN_DEFAULT = GAIN_PRECISION;
  //! Parameter name
  const auto GAIN = PREFIX + "gain"_id;
  //@}

  //@{
  const IntType SILENCE_LIMIT_PRECISION = 1;

  //! Default value - no silence detection
  const IntType SILENCE_LIMIT_DEFAULT = 0;
  //! Parameter name
  const auto SILENCE_LIMIT = PREFIX + "silencelimit"_id;
  //@}
}  // namespace Parameters::ZXTune::Sound
