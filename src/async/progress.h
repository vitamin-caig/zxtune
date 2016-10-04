/**
* 
* @file
*
* @brief Asynchronous progress interface and factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//std includes
#include <memory>

namespace Async
{
  class Progress
  {
  public:
    typedef std::shared_ptr<Progress> Ptr;
    virtual ~Progress() {}

    virtual void Produce(uint_t items) = 0;
    virtual void Consume(uint_t items) = 0;
    virtual void WaitForComplete() const = 0;

    static Ptr Create();
  };
}
