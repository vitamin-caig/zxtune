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
    return hours ? Strings::Format("{0}:{1:02}:{2:02}.{3:02}", hours, minutes, seconds, frames)
                 : Strings::Format("{0}:{1:02}.{2:02}", minutes, seconds, frames);
  }
}  // namespace Strings
