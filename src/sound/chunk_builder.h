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

//library includes
#include <sound/chunk.h>
//boost includes
#include <boost/make_shared.hpp>

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
      assert(maxSize < 1048576);
      Content = boost::make_shared<Chunk>(maxSize);
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

    Chunk::Ptr GetResult()
    {
      Content->resize(Pos - &Content->front());
      return Content;
    }
  private:
    Chunk::Ptr Content;
    Sample* Pos;
  };
}
