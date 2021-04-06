/**
 *
 * @file
 *
 * @brief  HES containers support
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
  Decoder::Ptr CreateHESDecoder()
  {
    static const Char DESCRIPTION[] = "Multi-HES";
    const auto decoder = Formats::Multitrack::CreateHESDecoder();
    return CreateMultitrackArchiveDecoder(DESCRIPTION, decoder);
  }
}  // namespace Formats::Archived
