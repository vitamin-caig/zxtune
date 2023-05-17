/**
 *
 * @file
 *
 * @brief  State interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <time/duration.h>
#include <time/instant.h>
// std includes
#include <memory>

namespace Module
{
  //! @brief Runtime module status
  class State
  {
  public:
    //! Pointer type
    using Ptr = std::shared_ptr<const State>;

    virtual ~State() = default;

    //! Current playback position till Information::Duration
    virtual Time::AtMillisecond At() const = 0;

    //! Total played time ignoring seeks
    virtual Time::Milliseconds Total() const = 0;

    //! Count of restarts due to looping
    virtual uint_t LoopCount() const = 0;
  };
}  // namespace Module
