/**
 *
 * @file
 *
 * @brief Interface of asynchronous activity
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "error.h"

namespace Async
{
  class Operation
  {
  public:
    using Ptr = std::shared_ptr<Operation>;
    virtual ~Operation() = default;

    virtual void Prepare() = 0;
    virtual void Execute() = 0;
  };

  class Activity
  {
  public:
    using Ptr = std::shared_ptr<Activity>;
    virtual ~Activity() = default;

    virtual bool IsExecuted() const = 0;
    //! @throw Error if Operation execution failed
    virtual void Wait() = 0;

    static Ptr Create(Operation::Ptr operation);
    static Ptr CreateStub();
  };
}  // namespace Async
