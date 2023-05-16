/**
 *
 * @file
 *
 * @brief  DigitalStudio support interface
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
  namespace DigitalStudio
  {
    typedef Digital::Builder Builder;

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
  }  // namespace DigitalStudio

  Decoder::Ptr CreateDigitalStudioDecoder();
}  // namespace Formats::Chiptune
