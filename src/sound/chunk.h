/**
*
* @file      sound/chunk.h
* @brief     Declaration of sound chunk
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_CHUNK_H_DEFINED
#define SOUND_CHUNK_H_DEFINED

//library includes
#include <sound/sample.h>
//std includes
#include <vector>
#include <cstring>

namespace Sound
{
  //! @brief Block of sound data
  struct Chunk : public std::vector<Sample>
  {
    Chunk()
    {
    }

    explicit Chunk(std::size_t size)
      : std::vector<Sample>(size)
    {
    }

    void ToS16(void* target) const
    {
      std::memcpy(target, &front(), size() * sizeof(front()));
    }

    void ToS16() {}

    void ToU8(void*) const
    {
      assert(!"Should not be called");
    }

    void ToU8()
    {
      assert(!"Should not be called");
    }
  };
}

#endif //SOUND_CHUNK_H_DEFINED
