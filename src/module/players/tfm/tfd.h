/**
* 
* @file
*
* @brief  TFD chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "module/players/tfm/tfm_factory.h"

namespace Module
{
  namespace TFD
  {
    TFM::Factory::Ptr CreateFactory();
  }
}
