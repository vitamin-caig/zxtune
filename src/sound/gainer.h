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
  class GainSource
  {
  public:
    typedef std::shared_ptr<const GainSource> Ptr;
    
    virtual Gain::Type Get() const = 0;
  };
  
  Receiver::Ptr CreateGainer(GainSource::Ptr gain, Receiver::Ptr delegate);
}
