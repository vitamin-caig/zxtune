/**
*
* @file      sound/sound_types.h
* @brief     Sound-related types and definitions
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SOUND_TYPES_H_DEFINED__
#define __SOUND_TYPES_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <vector>
//boost includes
#include <boost/array.hpp>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Output channels count
    const std::size_t OUTPUT_CHANNELS = 2;

    //! @brief Sample type
    typedef uint16_t Sample;
    //! @brief All-channels sample type
    typedef boost::array<Sample, OUTPUT_CHANNELS> MultiSample;

    // Sample attributes (better to specify exactly)
    const Sample SAMPLE_MIN = 0;
    const Sample SAMPLE_MID = 32768;
    const Sample SAMPLE_MAX = 65535;

    //! @brief Gain type
    typedef double Gain;
    //! @brief All-channels gain type
    typedef boost::array<Gain, OUTPUT_CHANNELS> MultiGain;

    //! @brief Block of sound data
    typedef std::vector<MultiSample> Chunk;
  }
}

#endif //__SOUND_TYPES_H_DEFINED__
