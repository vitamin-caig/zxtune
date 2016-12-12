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
#include <sound/sample.h>
//std includes
#include <cassert>
#include <cstring>
#include <vector>

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
    
    Chunk(const Chunk&) = delete;
    Chunk& operator = (const Chunk&) = delete;
    Chunk(Chunk&& rh)// = default
      : std::vector<Sample>(std::move(rh))
    {
    }
    
    Chunk& operator = (Chunk&& rh)// = default
    {
      std::vector<Sample>::operator = (std::move(rh));
      return *this;
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
