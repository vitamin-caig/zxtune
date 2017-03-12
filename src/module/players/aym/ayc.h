/**
* 
* @file
*
* @brief  AYC chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_factory.h"

namespace Module
{
  namespace AYC
  {
    AYM::Factory::Ptr CreateFactory();
  }
}
