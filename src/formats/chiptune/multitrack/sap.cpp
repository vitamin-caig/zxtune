/**
 *
 * @file
 *
 * @brief  SAP chiptunes support
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
    Decoder::Ptr CreateSAPDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      static const Char DESCRIPTION[] = "Slight Atari Player Sound Format";
      return CreateMultitrackChiptuneDecoder(DESCRIPTION, decoder);
    }
  }  // namespace Chiptune
}  // namespace Formats
