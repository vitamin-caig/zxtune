/*
Abstract:
  Chunks cache helper class

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_DETAILS_CHUNKS_CACHE_H_DEFINED
#define DEVICES_DETAILS_CHUNKS_CACHE_H_DEFINED

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

#endif //DEVICES_DETAILS_CHUNKS_CACHE_H_DEFINED
