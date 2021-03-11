/**
 *
 * @file
 *
 * @brief  Multitrack chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <formats/chiptune.h>
#include <formats/multitrack.h>

namespace Formats
{
  namespace Chiptune
  {
    Formats::Chiptune::Container::Ptr CreateMultitrackChiptuneContainer(Formats::Multitrack::Container::Ptr delegate);
    Formats::Chiptune::Decoder::Ptr CreateMultitrackChiptuneDecoder(const String& description,
                                                                    Formats::Multitrack::Decoder::Ptr delegate);
  }  // namespace Chiptune
}  // namespace Formats
