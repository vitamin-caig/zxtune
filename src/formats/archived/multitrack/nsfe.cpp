/**
 *
 * @file
 *
 * @brief  NSFE containers support
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
  Decoder::Ptr CreateNSFEDecoder()
  {
    static const Char DESCRIPTION[] = "Multi-NSFE";
    const auto decoder = Formats::Multitrack::CreateNSFEDecoder();
    return CreateMultitrackArchiveDecoder(DESCRIPTION, decoder);
  }
}  // namespace Formats::Archived
