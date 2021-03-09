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
// text includes
#include <formats/text/archived.h>

namespace Formats
{
  namespace Archived
  {
    Decoder::Ptr CreateSAPDecoder()
    {
      const Formats::Multitrack::Decoder::Ptr decoder = Formats::Multitrack::CreateSAPDecoder();
      return CreateMultitrackArchiveDecoder(Text::SAP_ARCHIVE_DECODER_DESCRIPTION, decoder);
    }
  }  // namespace Archived
}  // namespace Formats
