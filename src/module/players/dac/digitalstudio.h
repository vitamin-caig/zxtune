/**
* 
* @file
*
* @brief  DigitalStudio chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "dac_factory.h"

namespace Module
{
  namespace DigitalStudio
  {
    DAC::Factory::Ptr CreateFactory();
  }
}
