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

#include "sound/sample.h"  // IWYU pragma: export

#include <cassert>
#include <cstring>
#include <vector>

namespace Sound
{
  //! @brief Block of sound data
  struct Chunk : public std::vector<Sample>
  {
    Chunk() = default;

    explicit Chunk(std::size_t size)
      : std::vector<Sample>(size)
    {}

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&& rh) noexcept = default;
    Chunk& operator=(Chunk&& rh) noexcept = default;

    using iterator = Sample*;
    using const_iterator = const Sample*;

    iterator begin()
    {
      return data();
    }

    const_iterator begin() const
    {
      return data();
    }

    iterator end()
    {
      return data() + size();
    }

    const_iterator end() const
    {
      return data() + size();
    }

    void ToS16(void* target) const
    {
      std::memcpy(target, data(), size() * sizeof(front()));
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
}  // namespace Sound
