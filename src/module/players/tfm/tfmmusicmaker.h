/**
* 
* @file
*
* @brief  TFMMusicMaker chiptune factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "tfm_factory.h"
//library includes
#include <formats/chiptune/fm/tfmmusicmaker.h>

namespace Module
{
  namespace TFMMusicMaker
  {
    TFM::Factory::Ptr CreateFactory(Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder);
  }
}
