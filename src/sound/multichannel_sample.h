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

// library includes
#include <sound/sample.h>
// std includes
#include <array>

namespace Sound
{
  template<unsigned Channels>
  struct MultichannelSample
  {
    using Type = std::array<Sample::Type, Channels>;
  };
}  // namespace Sound
