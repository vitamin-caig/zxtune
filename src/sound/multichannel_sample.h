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
//boost includes
#include <boost/array.hpp>

namespace Sound
{
  template<unsigned Channels>
  struct MultichannelSample
  {
    typedef boost::array<Sample::Type, Channels> Type;
  };
}
