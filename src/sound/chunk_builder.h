/**
*
* @file
*
* @brief  Declaration of sound chunk
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <make_ptr.h>
//library includes
#include <sound/chunk.h>

namespace Sound
{
  //! @brief Block of sound data
  class ChunkBuilder
  {
  public:
    ChunkBuilder()
      : Pos()
    {
    }

    void Reserve(std::size_t maxSize)
    {
      Content.resize(maxSize);
      Pos = Content.data();
    }

    void Add(Sample smp)
    {
      assert(Pos <= &Content.back());
      *Pos++ = smp;
    }

    Sample* Allocate(std::size_t size)
    {
      Sample* res = Pos;
      Pos += size;
      assert(Pos <= &Content.back() + 1);
      return res;
    }

    Chunk CaptureResult()
    {
      Content.resize(Pos - Content.data());
      return std::move(Content);
    }
  private:
    Chunk Content;
    Sample* Pos;
  };
}
