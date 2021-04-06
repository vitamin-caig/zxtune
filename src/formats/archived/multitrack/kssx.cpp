/**
 *
 * @file
 *
 * @brief  KSSX containers support
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
  Decoder::Ptr CreateKSSXDecoder()
  {
    static const Char DESCRIPTION[] = "Multi-KSSX";
    const auto decoder = Formats::Multitrack::CreateKSSXDecoder();
    return CreateMultitrackArchiveDecoder(DESCRIPTION, decoder);
  }
}  // namespace Formats::Archived
