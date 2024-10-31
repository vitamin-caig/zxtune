/**
 *
 * @file
 *
 * @brief  Sound-related types and definitions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "sound/sample.h"

#include <array>

namespace Sound
{
  template<unsigned Channels>
  struct MultichannelSample
  {
    using Type = std::array<Sample::Type, Channels>;
  };
}  // namespace Sound
