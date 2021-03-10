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
// text includes
#include <strings/text/time_format.h>

namespace Strings
{
  String FormatTime(uint_t hours, uint_t minutes, uint_t seconds, uint_t frames)
  {
    return hours ? Strings::Format(Text::TIME_FORMAT_HOURS, hours, minutes, seconds, frames)
                 : Strings::Format(Text::TIME_FORMAT, minutes, seconds, frames);
  }
}  // namespace Strings
