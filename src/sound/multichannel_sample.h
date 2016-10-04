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

//library includes
#include <sound/sample.h>
//std includes
#include <array>

namespace Sound
{
  template<unsigned Channels>
  struct MultichannelSample
  {
    typedef std::array<Sample::Type, Channels> Type;
  };
}
