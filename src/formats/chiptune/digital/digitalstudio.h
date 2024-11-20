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

#include "formats/chiptune/digital/digital.h"

#include "formats/chiptune.h"

namespace Formats::Chiptune
{
  namespace DigitalStudio
  {
    using Builder = Digital::Builder;

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
  }  // namespace DigitalStudio

  Decoder::Ptr CreateDigitalStudioDecoder();
}  // namespace Formats::Chiptune
