/**
* 
* @file
*
* @brief  Song length database implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "songlengths.h"
//common includes
#include <contract.h>
#include <crc.h>
#include <pointers.h>
//std includes
#include <algorithm>

namespace Module
{
namespace Sid
{
  struct SongEntry
  {
    const uint32_t HashCrc32;
    const uint32_t Seconds;
    
    bool operator < (uint32_t hashCrc32) const
    {
      return HashCrc32 < hashCrc32;
    }
  };
  
  const SongEntry SONGS[] =
  {
#include "songlengths_db.inc"
  };

  TimeType GetSongLength(const char* md5digest, uint_t idx)
  {
    const uint32_t hashCrc32 = Crc32(safe_ptr_cast<const uint8_t*>(md5digest), 32);
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
    return TimeType();
  }
}//namespace Sid
}//namespace Module
