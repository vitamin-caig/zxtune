/**
* 
* @file
*
* @brief  PSG chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_factory.h"

namespace Module
{
  namespace PSG
  {
    AYM::Factory::Ptr CreateFactory();
  }
}
