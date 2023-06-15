/**
 *
 * @file
 *
 * @brief  Song length database implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/players/sid/songlengths.h"
// common includes
#include <contract.h>
#include <pointers.h>
// library includes
#include <binary/crc.h>
// std includes
#include <algorithm>

namespace Module::Sid
{
  struct SongEntry
  {
    const uint32_t HashCrc32;
    const uint32_t Seconds;

    bool operator<(uint32_t hashCrc32) const
    {
      return HashCrc32 < hashCrc32;
    }
  };

  const SongEntry SONGS[] = {
#include "core/plugins/players/sid/songlengths_db.inc"
  };

  TimeType GetSongLength(const char* md5digest, uint_t idx)
  {
    const uint32_t hashCrc32 = Binary::Crc32(Binary::View(md5digest, 32));
    const SongEntry* const end = std::end(SONGS);
    const SongEntry* const lower = std::lower_bound(SONGS, end, hashCrc32);
    if (lower + idx < end && lower->HashCrc32 == hashCrc32)
    {
      const SongEntry* const entry = lower + idx;
      if (lower->HashCrc32 == entry->HashCrc32)
      {
        return Time::Seconds(entry->Seconds);
      }
    }
    return {};
  }
}  // namespace Module::Sid
