/**
* 
* @file
*
* @brief  NSF containers support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "multitrack.h"
//library includes
#include <formats/multitrack/decoders.h>
//text includes
#include <formats/text/archived.h>

namespace Formats
{
  namespace Archived
  {
    Decoder::Ptr CreateNSFDecoder()
    {
      const Formats::Multitrack::Decoder::Ptr decoder = Formats::Multitrack::CreateNSFDecoder();
      return CreateMultitrackArchiveDecoder(Text::NSF_ARCHIVE_DECODER_DESCRIPTION, decoder);
    }
  }
}
