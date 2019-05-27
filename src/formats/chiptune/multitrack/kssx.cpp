/**
* 
* @file
*
* @brief  KSS chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/chiptune/multitrack/multitrack.h"
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    Decoder::Ptr CreateKSSXDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      return CreateMultitrackChiptuneDecoder(Text::KSSX_DECODER_DESCRIPTION, decoder);
    }
  }
}
