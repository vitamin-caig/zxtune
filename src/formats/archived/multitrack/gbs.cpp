/**
 *
 * @file
 *
 * @brief  GBE containers support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/multitrack/multitrack.h"
// library includes
#include <formats/multitrack/decoders.h>

namespace Formats::Archived
{
  Decoder::Ptr CreateGBSDecoder()
  {
    static const Char DESCRIPTION[] = "Multi-GBS";
    const auto decoder = Formats::Multitrack::CreateGBSDecoder();
    return CreateMultitrackArchiveDecoder(DESCRIPTION, decoder);
  }
}  // namespace Formats::Archived
