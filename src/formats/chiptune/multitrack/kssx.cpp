/**
 *
 * @file
 *
 * @brief  KSS chiptunes support
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
    Decoder::Ptr CreateKSSXDecoder(Formats::Multitrack::Decoder::Ptr decoder)
    {
      static const Char DESCRIPTION[] = "KSS Extended Music Format";
      return CreateMultitrackChiptuneDecoder(DESCRIPTION, decoder);
    }
  }  // namespace Chiptune
}  // namespace Formats
