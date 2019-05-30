/**
* 
* @file
*
* @brief  GBE containers support
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
    Decoder::Ptr CreateGBSDecoder()
    {
      const Formats::Multitrack::Decoder::Ptr decoder = Formats::Multitrack::CreateGBSDecoder();
      return CreateMultitrackArchiveDecoder(Text::GBS_ARCHIVE_DECODER_DESCRIPTION, decoder);
    }
  }
}
