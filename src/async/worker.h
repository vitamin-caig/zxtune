/**
*
* @file     async/worker.h
* @brief    Interface of quanted worker used to create asynchronous job
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ASYNC_WORKER_H_DEFINED
#define ASYNC_WORKER_H_DEFINED

//library includes
#include <async/job.h>

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
    typedef boost::shared_ptr<Worker> Ptr;
    virtual ~Worker() {}

    virtual void Initialize() = 0;
    virtual void Finalize() = 0;

    virtual void Suspend() = 0;
    virtual void Resume() = 0;

    virtual bool IsFinished() const = 0;
    virtual void ExecuteCycle() = 0;
  };

  Job::Ptr CreateJob(Worker::Ptr worker);
}

#endif //ASYNC_WORKER_H_DEFINED
