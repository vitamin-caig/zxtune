/**
 *
 * @file
 *
 * @brief  Song length database interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "time/duration.h"

#include "string_view.h"
#include "types.h"

namespace Module::Sid
{
  using TimeType = Time::Milliseconds;

  TimeType GetSongLength(StringView md5digest, uint_t idx);
}  // namespace Module::Sid
