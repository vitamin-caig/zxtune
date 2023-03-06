/**
 *
 * @file
 *
 * @brief  Loop parameters
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>

namespace Module
{
  struct LoopParameters
  {
    bool Enabled = false;
    uint_t Limit = 0;

    LoopParameters() = default;
    LoopParameters(bool enabled, uint_t limit)
      : Enabled(enabled)
      , Limit(limit)
    {}

    bool operator()(uint_t loopCount) const
    {
      return !loopCount || (Enabled && (!Limit || loopCount < Limit));
    }
  };
}  // namespace Module
