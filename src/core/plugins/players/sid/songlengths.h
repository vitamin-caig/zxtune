/**
 *
 * @file
 *
 * @brief  Song length database interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <string_view.h>
// library includes
#include <time/duration.h>

namespace Module::Sid
{
  using TimeType = Time::Milliseconds;

  TimeType GetSongLength(StringView md5digest, uint_t idx);
}  // namespace Module::Sid
