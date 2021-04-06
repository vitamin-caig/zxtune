/**
 *
 * @file
 *
 * @brief  Format functions implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <strings/format.h>

namespace Strings
{
  String FormatTime(uint_t hours, uint_t minutes, uint_t seconds, uint_t frames)
  {
    return hours ? Strings::Format("%1%:%2$02u:%3$02u.%4$02u", hours, minutes, seconds, frames)
                 : Strings::Format("%1%:%2$02u.%3$02u", minutes, seconds, frames);
  }
}  // namespace Strings
