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

    virtual void Prepare() = 0;
    virtual void Execute() = 0;
  };

  class Activity
  {
  public:
    typedef boost::shared_ptr<Activity> Ptr;
    virtual ~Activity() {}

    virtual bool IsExecuted() const = 0;
    //! @throw Error if Operation execution failed
    virtual void Wait() = 0;

    static Ptr Create(Operation::Ptr operation);
    static Ptr CreateStub();
  };
}

#endif //__ASYNC_ACTIVITY_H_DEFINED__
