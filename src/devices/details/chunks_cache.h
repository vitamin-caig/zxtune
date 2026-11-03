/**
 *
 * @file
 *
 * @brief  Data chunks cache helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "contract.h"

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

namespace Devices::Details
{
  template<class ChunkType, class StampType>
  class ChunksCache
  {
  public:
    void Add(ChunkType&& src)
    {
      // Reuse the capacity occupied by the consumed prefix instead of growing the storage
      if (Consumed && Buffer.capacity() == Buffer.size())
      {
        const auto pending = Buffer.size() - Consumed;
        std::move(Buffer.begin() + Consumed, Buffer.end(), Buffer.begin());
        Buffer.erase(Buffer.begin() + pending, Buffer.end());
        Consumed = 0;
      }
      Require(Buffer.empty() || Buffer.back().TimeStamp < src.TimeStamp);
      Buffer.emplace_back(std::move(src));
    }

    // Returns the consumed chunks; the span is valid until the next Add/ConsumeBefore call
    std::span<const ChunkType> ConsumeBefore(StampType till)
    {
      const auto avail = std::span<const ChunkType>(Buffer).subspan(Consumed);
      const auto end =
          std::find_if(avail.begin(), avail.end(), [till](const auto& it) { return !(it.TimeStamp < till); });
      const auto count = static_cast<std::size_t>(end - avail.begin());
      Consumed += count;
      return std::span<const ChunkType>(avail.data(), count);
    }

  private:
    std::vector<ChunkType> Buffer;
    std::size_t Consumed = 0;
  };
}  // namespace Devices::Details
