/**
* 
* @file
*
* @brief  DigitalMusicMaker chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "dac_factory.h"

namespace Module
{
  namespace DigitalMusicMaker
  {
    DAC::Factory::Ptr CreateFactory();
  }
}
