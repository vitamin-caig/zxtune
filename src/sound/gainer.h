/**
*
* @file      sound/gainer.h
* @brief     Defenition of gain-related functionality
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_GAINER_H_DEFINED
#define SOUND_GAINER_H_DEFINED

//library includes
#include <sound/receiver.h>

namespace Sound
{
  //! @brief Gain control interface
  class FadeGainer : public Converter
  {
  public:
    //! @brief Pointer type
    typedef boost::shared_ptr<FadeGainer> Ptr;

    //! @brief Setting up the gain value
    //! @param gain Levels
    //! @throw Error
    virtual void SetGain(double gain) = 0;
    
    //! @brief Setting up the fading
    //! @param delta Gain changing
    //! @param step Samples count to apply delta
    //! @throw Error
    virtual void SetFading(double delta, uint_t step) = 0;
  };

  //! @brief Creating gainer insance
  FadeGainer::Ptr CreateFadeGainer();
}

#endif //SOUND_GAINER_H_DEFINED
