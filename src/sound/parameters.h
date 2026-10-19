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

#include "parameters/value.h"

namespace Parameters::ZXTune::Sound
{
  constexpr auto Prefix = ZXTune::PREFIX + "sound"_id;

  //! @brief %Sound frequency in Hz
  struct Frequency : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "frequency"_id;
    static constexpr Type Default = 44100;
  };

  //! @brief Looped playback
  struct Looped : Value<bool>
  {
    static constexpr auto Key = Prefix + "looped"_id;
    static constexpr Type Default = false;
  };

  //! @brief Max loops count, unlimited if zero
  struct LoopLimit : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "looplimit"_id;
    static constexpr Type Default = 0, Min = 0, Max = 10;
  };

  //! @brief Fadein in seconds
  struct FadeIn : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "fadein"_id;
    static constexpr Type Default = 0, Min = 0, Max = 30;
    //! @brief ticks per second
    static constexpr IntType Precision = 1;
  };

  //! @brief Fadeout in seconds
  struct FadeOut : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "fadeout"_id;
    static constexpr Type Default = 0, Min = 0, Max = 30;
    //! @brief ticks per second
    static constexpr IntType Precision = 1;
  };

  //! @brief Gain in percents
  struct Gain : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "gain"_id;
    //! @brief percents
    static constexpr IntType Precision = 100;
    static constexpr Type Default = Precision, Min = 1, Max = 1000;
  };

  //! @brief Silence detection limit in seconds
  struct SilenceLimit : Value<uint_t>
  {
    static constexpr auto Key = Prefix + "silencelimit"_id;
    static constexpr Type Default = 0, Min = 0, Max = 30;
    //! @brief ticks per second
    static constexpr IntType Precision = 1;
  };
}  // namespace Parameters::ZXTune::Sound
