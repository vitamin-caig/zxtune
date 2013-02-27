/**
*
* @file     async/job.h
* @brief    Interface of asynchronous job
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ASYNC_JOB_H_DEFINED
#define ASYNC_JOB_H_DEFINED

//boost includes
#include <boost/shared_ptr.hpp>

class Error;

namespace Async
{
  class Job
  {
  public:
    typedef boost::shared_ptr<Job> Ptr;
    virtual ~Job() {}

    virtual void Start() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;

    virtual bool IsActive() const = 0;
    virtual bool IsPaused() const = 0;
  };
}

#endif //ASYNC_JOB_H_DEFINED
