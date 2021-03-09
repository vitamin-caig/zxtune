/**
 *
 * @file
 *
 * @brief  NSF chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/multitrack/multitrack.h"
// text includes
#include <formats/text/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    Decoder::Ptr CreateNSFDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      return CreateMultitrackChiptuneDecoder(Text::NSF_DECODER_DESCRIPTION, decoder);
    }
  }  // namespace Chiptune
}  // namespace Formats
