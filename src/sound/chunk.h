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
//boost includes
#include <boost/shared_ptr.hpp>

namespace Sound
{
  //! @brief Block of sound data
  struct Chunk : public std::vector<Sample>
  {
    typedef boost::shared_ptr<Chunk> Ptr;

    Chunk()
    {
    }

    explicit Chunk(std::size_t size)
      : std::vector<Sample>(size)
    {
    }

    typedef Sample* iterator;
    typedef const Sample* const_iterator;

    iterator begin()
    {
      return &front();
    }

    const_iterator begin() const
    {
      return &front();
    }

    iterator end()
    {
      return &back() + 1;
    }

    const_iterator end() const
    {
      return &back() + 1;
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
