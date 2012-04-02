/**
*
* @file     async/progress.h
* @brief    Asynchronous progress interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ASYNC_PROGRESS_H_DEFINED
#define ASYNC_PROGRESS_H_DEFINED

//common includes
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Async
{
  class Progress
  {
  public:
    typedef boost::shared_ptr<Progress> Ptr;
    virtual ~Progress() {}

    virtual void Produce(uint_t items) = 0;
    virtual void Consume(uint_t items) = 0;
    virtual void WaitForComplete() const = 0;

    static Ptr Create();
  };
}

#endif //ASYNC_PROGRESS_H_DEFINED
