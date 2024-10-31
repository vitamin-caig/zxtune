/**
 *
 * @file
 *
 * @brief Asynchronous progress interface and factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "types.h"

#include <memory>

namespace Async
{
  class Progress
  {
  public:
    using Ptr = std::shared_ptr<Progress>;
    virtual ~Progress() = default;

    virtual void Produce(uint_t items) = 0;
    virtual void Consume(uint_t items) = 0;
    virtual void WaitForComplete() const = 0;

    static Ptr Create();
  };
}  // namespace Async
