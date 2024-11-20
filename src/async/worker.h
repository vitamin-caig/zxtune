/**
 *
 * @file
 *
 * @brief Interface of quanted worker used to create asynchronous job
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "async/job.h"

namespace Async
{
  /*
                      +----------------------+
                      V                      ^
    Initialize -> IsFinished -> Suspend -> Resume -> Finalize
                  V        ^      |          |          ^
                 ExecuteCycle -e->+---e----->+---e----->+
  */
  class Worker
  {
  public:
    using Ptr = std::shared_ptr<Worker>;
    virtual ~Worker() = default;

    virtual void Initialize() = 0;
    virtual void Finalize() = 0;

    virtual void Suspend() = 0;
    virtual void Resume() = 0;

    virtual bool IsFinished() const = 0;
    virtual void ExecuteCycle() = 0;
  };

  Job::Ptr CreateJob(Worker::Ptr worker);
}  // namespace Async
