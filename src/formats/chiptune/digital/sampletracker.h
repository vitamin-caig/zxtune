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

#include "formats/chiptune/digital/digital.h"

#include "formats/chiptune.h"

namespace Formats::Chiptune
{
  namespace SampleTracker
  {
    using Builder = Digital::Builder;

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
  }  // namespace SampleTracker

  Decoder::Ptr CreateSampleTrackerDecoder();
}  // namespace Formats::Chiptune
