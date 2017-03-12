/**
* 
* @file
*
* @brief  YM/VTX chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_factory.h"
//library includes
#include <formats/chiptune/aym/ym.h>

namespace Module
{
  namespace YMVTX
  {
    AYM::Factory::Ptr CreateFactory(Formats::Chiptune::YM::Decoder::Ptr decoder);
  }
}
