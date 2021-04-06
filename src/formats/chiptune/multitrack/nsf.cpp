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

namespace Formats
{
  namespace Chiptune
  {
    Decoder::Ptr CreateNSFDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      static const Char DESCRIPTION[] = "NES Sound Format";
      return CreateMultitrackChiptuneDecoder(DESCRIPTION, decoder);
    }
  }  // namespace Chiptune
}  // namespace Formats
