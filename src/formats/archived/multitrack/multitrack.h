/**
 *
 * @file
 *
 * @brief  Multitrack archives support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <formats/archived.h>
#include <formats/multitrack.h>

namespace Formats::Archived
{
  Decoder::Ptr CreateMultitrackArchiveDecoder(String description, Formats::Multitrack::Decoder::Ptr delegate);
}  // namespace Formats::Archived
