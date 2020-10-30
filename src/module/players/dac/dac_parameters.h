/**
* 
* @file
*
* @brief  DAC-based parameters helpers
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/dac.h>
#include <parameters/accessor.h>

namespace Module
{
  namespace DAC
  {
    Devices::DAC::ChipParameters::Ptr CreateChipParameters(uint_t samplerate, Parameters::Accessor::Ptr params);
  }
}
