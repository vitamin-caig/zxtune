/*
Abstract:
  Asynchronous job implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <error.h>
//library includes
#include <async/coroutine.h>
#include <async/worker.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  class WorkerCoroutine : public Async::Coroutine
  {
  public:
    explicit WorkerCoroutine(Async::Worker::Ptr worker)
      : Delegate(worker)
    {
    }

    virtual Error Initialize()
    {
      return Delegate->Initialize();
    }

    virtual Error Finalize()
    {
      return Delegate->Finalize();
    }

    virtual Error Suspend()
    {
      return Delegate->Suspend();
    }

    virtual Error Resume()
    {
      return Delegate->Resume();
    }

    virtual Error Execute(Async::Scheduler& sch)
    {
      while (!Delegate->IsFinished())
      {
        if (const Error& e = Delegate->ExecuteCycle())
        {
          return e;
        }
        sch.Yield();
      }
      return Error();
    }
  private:
    const Async::Worker::Ptr Delegate;
  };
}

namespace Async
{
  Job::Ptr CreateJob(Worker::Ptr worker)
  {
    const Coroutine::Ptr routine = boost::make_shared<WorkerCoroutine>(worker);
    return CreateJob(routine);
  }
}