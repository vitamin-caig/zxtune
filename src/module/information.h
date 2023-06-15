/**
 *
 * @file
 *
 * @brief  Information interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <time/duration.h>
// std includes
#include <memory>

namespace Module
{
  //! @brief Common module information
  class Information
  {
  public:
    //! Pointer type
    using Ptr = std::shared_ptr<const Information>;

    virtual ~Information() = default;

    //! Total module duration
    virtual Time::Milliseconds Duration() const = 0;

    //! Loop duration
    virtual Time::Milliseconds LoopDuration() const = 0;
  };
}  // namespace Module
