/**
 *
 * @file
 *
 * @brief  GBS chiptunes support
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
    Decoder::Ptr CreateGBSDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      return CreateMultitrackChiptuneDecoder(Text::GBS_DECODER_DESCRIPTION, decoder);
    }
  }  // namespace Chiptune
}  // namespace Formats
