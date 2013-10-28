/**
*
* @file      sound/sample_multichannel.h
* @brief     Sound-related types and definitions
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_SAMPLE_MULTICHANNEL_H_DEFINED
#define SOUND_SAMPLE_MULTICHANNEL_H_DEFINED

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

#endif //SOUND_SAMPLE_MULTICHANNEL_H_DEFINED
