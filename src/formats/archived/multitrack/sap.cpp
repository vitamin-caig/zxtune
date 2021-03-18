/**
 *
 * @file
 *
 * @brief  SAP containers support
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
  Decoder::Ptr CreateSAPDecoder()
  {
    static const Char DESCRIPTION[] = "Multi-SAP";
    const auto decoder = Formats::Multitrack::CreateSAPDecoder();
    return CreateMultitrackArchiveDecoder(DESCRIPTION, decoder);
  }
}  // namespace Formats::Archived
