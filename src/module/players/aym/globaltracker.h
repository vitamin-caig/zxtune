/**
* 
* @file
*
* @brief  GlobalTracker chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_factory.h"

namespace Module
{
  namespace GlobalTracker
  {
    AYM::Factory::Ptr CreateFactory();
  }
}
