/**
 *
 * @file
 *
 * @brief  HES chiptunes support
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
    Decoder::Ptr CreateHESDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      static const Char DESCRIPTION[] = "Home Entertainment System";
      return CreateMultitrackChiptuneDecoder(DESCRIPTION, decoder);
    }
  }  // namespace Chiptune
}  // namespace Formats
