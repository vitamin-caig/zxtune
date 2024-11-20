/**
 *
 * @file
 *
 * @brief  Analyzer interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "math/fixedpoint.h"

#include "types.h"

#include <memory>

namespace Sound
{
  //! @brief %Sound analyzer interface
  class Analyzer
  {
  public:
    //! Pointer type
    using Ptr = std::shared_ptr<const Analyzer>;

    virtual ~Analyzer() = default;

    using LevelType = Math::FixedPoint<uint8_t, 100>;

    // TODO: use std::span when available
    virtual void GetSpectrum(LevelType* result, std::size_t limit) const = 0;
  };
}  // namespace Sound
