/**
*
* @file     async/activity.h
* @brief    Interface of asynchronous activity
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __ASYNC_ACTIVITY_H_DEFINED__
#define __ASYNC_ACTIVITY_H_DEFINED__

//common includes
#include <error.h>

namespace Async
{
  class Operation
  {
  public:
    typedef boost::shared_ptr<Operation> Ptr;
    virtual ~Operation() {}

    virtual Error Prepare() = 0;
    virtual Error Execute() = 0;
  };

  class Activity
  {
  public:
    typedef boost::shared_ptr<Activity> Ptr;
    virtual ~Activity() {}

    virtual bool IsExecuted() const = 0;
    virtual Error Wait() = 0;

    static Error Create(Operation::Ptr operation, Activity::Ptr& result);
  };
}

#endif //__ASYNC_ACTIVITY_H_DEFINED__
