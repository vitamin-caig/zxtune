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

// local includes
#include "formats/chiptune/digital/digital.h"
// library includes
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace SampleTracker
  {
    using Builder = Digital::Builder;

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
  }  // namespace SampleTracker

  Decoder::Ptr CreateSampleTrackerDecoder();
}  // namespace Formats::Chiptune
