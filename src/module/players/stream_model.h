/**
 *
 * @file
 *
 * @brief  Stream model definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// std includes
#include <memory>

namespace Module
{
  class StreamModel
  {
  public:
    using Ptr = std::shared_ptr<const StreamModel>;
    virtual ~StreamModel() = default;

    virtual uint_t GetTotalFrames() const = 0;
    virtual uint_t GetLoopFrame() const = 0;
  };
}  // namespace Module
