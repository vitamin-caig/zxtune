/**
*
* @file
*
* @brief  Defenition of gain-related functionality
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <sound/gain.h>
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
    virtual void SetGain(Gain::Type gain) = 0;
    
    //! @brief Setting up the fading
    //! @param delta Gain changing
    //! @param step Frames count to apply delta
    //! @throw Error
    virtual void SetFading(Gain::Type delta, uint_t step) = 0;
  };

  //! @brief Creating gainer insance
  FadeGainer::Ptr CreateFadeGainer();
}
