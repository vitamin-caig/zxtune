/**
* 
* @file
*
* @brief  KSSX containers support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/archived/multitrack/multitrack.h"
//library includes
#include <formats/multitrack/decoders.h>
//text includes
#include <formats/text/archived.h>

namespace Formats
{
  namespace Archived
  {
    Decoder::Ptr CreateKSSXDecoder()
    {
      const Formats::Multitrack::Decoder::Ptr decoder = Formats::Multitrack::CreateKSSXDecoder();
      return CreateMultitrackArchiveDecoder(Text::KSSX_ARCHIVE_DECODER_DESCRIPTION, decoder);
    }
  }
}
