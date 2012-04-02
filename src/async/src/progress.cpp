/*
Abstract:
  Progress implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <async/progress.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace
{
  class SynchronizedProgress : public Async::Progress
  {
  public:
    SynchronizedProgress()
      : Produced()
      , Consumed()
    {
    }

    virtual void Produce(uint_t items)
    {
      const boost::mutex::scoped_lock lock(Mutex);
      Produced += items;
    }

    virtual void Consume(uint_t items)
    {
      const boost::mutex::scoped_lock lock(Mutex);
      Consumed += items;
      NotifyIfComplete();
    }

    virtual void WaitForComplete() const
    {
      boost::mutex::scoped_lock lock(Mutex);
      Complete.wait(lock, boost::bind(&SynchronizedProgress::IsComplete, this));
    }
  private:
    void NotifyIfComplete()
    {
      if (IsComplete())
      {
        Complete.notify_all();
      }
    }

    bool IsComplete() const
    {
      return Produced == Consumed;
    }
  private:
    uint_t Produced;
    uint_t Consumed;
    mutable boost::mutex Mutex;
    mutable boost::condition_variable Complete;
  };
}

namespace Async
{
  Progress::Ptr Progress::Create()
  {
    return boost::make_shared<SynchronizedProgress>();
  }
}
