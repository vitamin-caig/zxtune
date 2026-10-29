/**
 *
 * @file
 *
 * @brief  SNDH song length database implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/atari/timedb.h"

#include "math/bitops.h"

#include <algorithm>
#include <cstddef>

namespace Module::Atari
{
#include "module/players/aym/atari/sndh.inc"

  constexpr uint32_t GROUP_SIZE_SHIFT = Math::Log2(VALUE_MASK);
  constexpr uint32_t GROUP_SIZE_MASK = ~VALUE_MASK;

  static uint32_t HashInternal(const uint8_t* data, std::size_t size, uint32_t hash)
  {
    for (std::size_t idx = 0; idx != size; ++idx)
    {
      hash += data[idx];
      hash += hash << 10;
      hash ^= hash >> 6;
    }
    return hash;
  }

  // sc68 parses the 32-byte header first, seeks back and then hashes the whole file
  static uint32_t Sc68Hash(Binary::View data)
  {
    if (data.Size() < 32)
    {
      return 0;
    }
    const auto* const raw = data.As<uint8_t>();
    const auto hash = HashInternal(raw, 32, 0);
    return HashInternal(raw, data.Size(), hash);
  }

  static uint_t ReadVarint(const uint8_t*& cursor)
  {
    uint_t result = 0;
    uint_t shift = 0;
    for (;;)
    {
      const uint8_t byte = *cursor++;
      result |= uint_t(byte & 0x7f) << shift;
      if (!(byte & 0x80))
      {
        return result;
      }
      shift += 7;
    }
  }

  static std::optional<uint_t> DecodeGroup(uint_t offset, uint_t count, uint_t idx)
  {
    if (idx >= count)
    {
      return {};
    }
    const auto* cursor = GROUPS + offset;
    for (uint_t pos = 0; pos != idx; ++pos)
    {
      ReadVarint(cursor);
    }
    const auto duration = ReadVarint(cursor);
    // Zero marks a subsong missing from the database
    return duration ? std::optional<uint_t>(duration) : std::nullopt;
  }

  std::optional<uint_t> GetSndhFrames(Binary::View module, uint_t idx)
  {
    const auto hash = Sc68Hash(module);
    const auto* const end = std::end(ENTRIES);
    const auto* const it = std::upper_bound(ENTRIES, end, uint64_t(hash) << 32);
    if (it == end || (*it) >> 32 != hash)
    {
      return {};
    }
    const auto value = uint_t(*it & VALUE_MASK);
    const auto groupSize = uint_t((*it & GROUP_SIZE_MASK) >> GROUP_SIZE_SHIFT);
    if (groupSize)
    {
      return DecodeGroup(value, groupSize, idx);
    }
    return idx ? std::nullopt : std::optional<uint_t>(value);
  }
}  // namespace Module::Atari
