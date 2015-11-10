/**
* 
* @file
*
* @brief  SampleTracker support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "digital.h"
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace SampleTracker
    {
      typedef Digital::Builder Builder;

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    }

    Decoder::Ptr CreateSampleTrackerDecoder();
  }
}
