/**
*
* @file      sound/filter.h
* @brief     Defenition of filtering-related functionality
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SOUND_FILTER_H_DEFINED__
#define __SOUND_FILTER_H_DEFINED__

//common includes
#include <error.h>
//library includes
#include <sound/receiver.h>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief %Filter interface
    class Filter : public Converter
    {
    public:
      //! @brief Pointer type
      typedef boost::shared_ptr<Filter> Ptr;

      //! @brief Switching filter to bandpass mode
      //! @param freq Working sound frequency in Hz
      //! @param lowCutoff Low cutoff edge in Hz
      //! @param highCutoff High cutoff edge in Hz
      //! @return Error() in case of success
      virtual Error SetBandpassParameters(uint_t freq, uint_t lowCutoff, uint_t highCutoff) = 0;
    };

    //! @brief Creating FIR-filter instance
    //! @param order Filter order
    //! @param filter Reference to result value
    //! @return Error() in case of success
    Error CreateFIRFilter(uint_t order, Filter::Ptr& filter);
  }
}

#endif //__FILTER_H_DEFINED__
