/**
* 
* @file
*
* @brief  SQ-Tracker chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "module/players/aym/aym_factory.h"

namespace Module
{
  namespace SQTracker
  {
    AYM::Factory::Ptr CreateFactory();
  }
}
