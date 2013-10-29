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

//std includes
#include <vector>

namespace Devices
{
  namespace Details
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
        return &Buffer.front();
      }
      
      const ChunkType* GetEnd() const
      {
        return &Buffer.back() + 1;
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
  }
}
