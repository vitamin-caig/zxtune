/**
* 
* @file
*
* @brief Interface of coroutine and job factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <async/job.h>

namespace Async
{
  class Scheduler
  {
  public:
    virtual ~Scheduler() {}

    virtual void Yield() = 0;
  };

  class Coroutine
  {
  public:
    typedef std::shared_ptr<Coroutine> Ptr;
    virtual ~Coroutine() {}

    virtual void Initialize() = 0;
    virtual void Finalize() = 0;

    virtual void Suspend() = 0;
    virtual void Resume() = 0;

    virtual void Execute(Scheduler& sch) = 0;
  };

  Job::Ptr CreateJob(Coroutine::Ptr routine);
}
