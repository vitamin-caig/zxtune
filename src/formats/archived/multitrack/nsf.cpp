/**
 *
 * @file
 *
 * @brief  NSF containers support
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
  Decoder::Ptr CreateNSFDecoder()
  {
    static const Char DESCRIPTION[] = "Multi-NSF";
    const auto decoder = Formats::Multitrack::CreateNSFDecoder();
    return CreateMultitrackArchiveDecoder(DESCRIPTION, decoder);
  }
}  // namespace Formats::Archived
