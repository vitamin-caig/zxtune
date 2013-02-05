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
#include <tools.h>
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
    typedef boost::array<Sample, OUTPUT_CHANNELS> OutputSample;

    template<unsigned Channels>
    class FixedChannelsSample : public boost::array<Sample, Channels> {};

    typedef std::vector<Sample> MultichannelSample;

    // Sample attributes (better to specify exactly)
    const Sample SAMPLE_MIN = 0;
    const Sample SAMPLE_MID = 32768;
    const Sample SAMPLE_MAX = 65535;

    const bool SAMPLE_SIGNED = SAMPLE_MID == 0;

    inline void ChangeSignCopy(const OutputSample* begin, const OutputSample* end, OutputSample* to)
    {
      BOOST_STATIC_ASSERT(sizeof(OutputSample) == 4);
      const uint32_t SIGN_MASK = 0x80008000;
      const uint32_t* it = safe_ptr_cast<const uint32_t*>(begin);
      const uint32_t* const lim = safe_ptr_cast<const uint32_t*>(end);
      uint32_t* res = safe_ptr_cast<uint32_t*>(to);
      for (; it != lim; ++it, ++res)
      {
        *res = *it ^ SIGN_MASK;
      }
    }

    //! @brief Gain type
    typedef double Gain;
    //! @brief All-channels gain type
    typedef boost::array<Gain, OUTPUT_CHANNELS> MultiGain;

    //! @brief Block of sound data
    struct Chunk : public std::vector<OutputSample>
    {
      Chunk()
      {
      }

      explicit Chunk(std::size_t size)
        : std::vector<OutputSample>(size)
      {
      }

      void ChangeSign()
      {
        ChangeSignCopy(&front(), &back() + 1, &front());
      }
    };
  }
}

#endif //__SOUND_TYPES_H_DEFINED__
