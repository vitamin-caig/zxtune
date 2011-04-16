/**
*
* @file     async/job.h
* @brief    Interface of asynchronous job
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __ASYNC_JOB_H_DEFINED__
#define __ASYNC_JOB_H_DEFINED__

//common includes
#include <error.h>

namespace Async
{
  class Job
  {
  public:
    typedef boost::shared_ptr<Job> Ptr;
    virtual ~Job() {}

    virtual Error Start() = 0;
    virtual Error Pause() = 0;
    virtual Error Stop() = 0;

    virtual bool IsActive() const = 0;
    virtual bool IsPaused() const = 0;

    class Worker
    {
    public:
      typedef boost::shared_ptr<Worker> Ptr;
      virtual ~Worker() {}

      virtual Error Initialize() = 0;
      virtual Error Finalize() = 0;

      virtual Error Suspend() = 0;
      virtual Error Resume() = 0;

      virtual Error ExecuteCycle() = 0;
      virtual bool IsFinished() const = 0;
    };

    static Ptr Create(Worker::Ptr worker);
  };
}

#endif //__ASYNC_JOB_H_DEFINED__
