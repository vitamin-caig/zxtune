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

#include <vector>

namespace Devices::Details
{
  template<class ChunkType, class StampType>
  class ChunksCache
  {
  public:
    void Add(const ChunkType& src)
    {
      Buffer.push_back(src);
    }

    const ChunkType* GetBegin() const
    {
      return Buffer.data();
    }

    const ChunkType* GetEnd() const
    {
      return GetBegin() + Buffer.size();
    }

    void Reset()
    {
      Buffer.clear();
    }

    StampType GetTillTime() const
    {
      return Buffer.empty() ? StampType() : Buffer.back().TimeStamp;
    }

  private:
    std::vector<ChunkType> Buffer;
  };
}  // namespace Devices::Details
