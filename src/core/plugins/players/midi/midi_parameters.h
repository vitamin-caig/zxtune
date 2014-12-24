/**
* 
* @file
*
* @brief  MIDI parameters helpers
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/midi.h>
#include <parameters/accessor.h>

namespace Module
{
  namespace MIDI
  {
    Devices::MIDI::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);
  }
}
