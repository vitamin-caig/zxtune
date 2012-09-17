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

    virtual Error Initialize() = 0;
    virtual Error Finalize() = 0;

    virtual Error Suspend() = 0;
    virtual Error Resume() = 0;

    virtual Error Execute(Scheduler& sch) = 0;
  };

  Job::Ptr CreateJob(Coroutine::Ptr routine);
}

#endif //ASYNC_COROUTINE_H_DEFINED
