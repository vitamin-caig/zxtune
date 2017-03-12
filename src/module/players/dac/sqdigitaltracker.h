/**
* 
* @file
*
* @brief  SQDigitalTracker chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "dac_factory.h"

namespace Module
{
  namespace SQDigitalTracker
  {
    DAC::Factory::Ptr CreateFactory();
  }
}
