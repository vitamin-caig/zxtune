/**
* 
* @file
*
* @brief  SoundTracker-based modules support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_factory.h"
//library includes
#include <formats/chiptune/aym/soundtracker.h>

namespace Module
{
  namespace SoundTracker
  {
    AYM::Factory::Ptr CreateFactory(Formats::Chiptune::SoundTracker::Decoder::Ptr decoder);
  }
}                                                     
