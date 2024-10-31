/**
 *
 * @file
 *
 * @brief  Song length database implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/sid/songlengths.h"

#include <binary/crc.h>

#include <string_view.h>

#include <algorithm>

namespace Module::Sid
{
#include "core/plugins/players/sid/songlengths_db.inc"

  TimeType DecodeGroup(std::size_t offset, uint_t idx)
  {
    const auto* src = GROUPS + offset;
    for (uint_t pos = 0;; ++pos)
    {
      uint_t val = *src++;
      if (val & 128)
      {
        val = (val & 127) | (uint_t(*src++) << 7);
      }
      if (pos == idx)
      {
        return Time::Seconds(val);
      }
    }
  }

  TimeType GetSongLength(StringView md5digest, uint_t idx)
  {
    const auto hashCrc32 = Binary::Crc32(Binary::View(md5digest.data(), md5digest.size()));
    const auto* const end = std::end(ENTRIES);
    const auto* const it = std::upper_bound(ENTRIES, end, uint64_t(hashCrc32) << 32);
    if (it == end || (*it) >> 32 != hashCrc32)
    {
      return {};
    }
    const auto value = *it & VALUE_MASK;
    const auto groupSize = (*it & GROUP_SIZE_MASK) >> GROUP_SIZE_SHIFT;
    if (!groupSize)
    {
      return idx != 0 ? TimeType{} : Time::Seconds(value);
    }
    else if (idx >= groupSize)
    {
      return {};
    }
    return DecodeGroup(value, idx);
  }
}  // namespace Module::Sid
