/**
* 
* @file
*
* @brief  TFM parameters helpers
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/tfm.h>
#include <parameters/accessor.h>

namespace Module
{
  namespace TFM
  {
    Devices::TFM::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);
  }
}
