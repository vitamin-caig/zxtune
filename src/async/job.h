/**
* 
* @file
*
* @brief Asynchronous job interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//boost includes
#include <boost/shared_ptr.hpp>

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
