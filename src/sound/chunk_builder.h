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
      Content = MakePtr<Chunk>(maxSize);
      Pos = &Content->front();
    }

    void Add(Sample smp)
    {
      assert(Pos <= &Content->back());
      *Pos++ = smp;
    }

    Sample* Allocate(std::size_t size)
    {
      Sample* res = Pos;
      Pos += size;
      assert(Pos <= &Content->back() + 1);
      return res;
    }

    Chunk::Ptr CaptureResult()
    {
      Content->resize(Pos - &Content->front());
      return std::move(Content);
    }
  private:
    Chunk::Ptr Content;
    Sample* Pos;
  };
}
