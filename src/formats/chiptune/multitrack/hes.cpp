/**
* 
* @file
*
* @brief  HES chiptunes support
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
    Decoder::Ptr CreateHESDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      return CreateMultitrackChiptuneDecoder(Text::HES_DECODER_DESCRIPTION, decoder);
    }
  }
}
