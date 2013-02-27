/**
*
* @file     async/coroutine.h
* @brief    Interface of quanted worker used to create asynchronous job
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ASYNC_COROUTINE_H_DEFINED
#define ASYNC_COROUTINE_H_DEFINED

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
    typedef boost::shared_ptr<Coroutine> Ptr;
    virtual ~Coroutine() {}

    virtual void Initialize() = 0;
    virtual void Finalize() = 0;

    virtual void Suspend() = 0;
    virtual void Resume() = 0;

    virtual void Execute(Scheduler& sch) = 0;
  };

  Job::Ptr CreateJob(Coroutine::Ptr routine);
}

#endif //ASYNC_COROUTINE_H_DEFINED
