/**
 *
 * @file
 *
 * @brief  NSFE chiptunes support
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
    Decoder::Ptr CreateNSFEDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      static const Char DESCRIPTION[] = "Extended Nintendo Sound Format";
      return CreateMultitrackChiptuneDecoder(DESCRIPTION, decoder);
    }
  }  // namespace Chiptune
}  // namespace Formats
