/**
* 
* @file
*
* @brief  SAP chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "multitrack.h"
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    Decoder::Ptr CreateSAPDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      return CreateMultitrackChiptuneDecoder(Text::SAP_DECODER_DESCRIPTION, decoder);
    }
  }
}
