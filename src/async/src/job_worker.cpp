/**
* 
* @file
*
* @brief Worker-based Job factory
*
* @author vitamin.caig@gmail.com
*
**/

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

    virtual void Initialize()
    {
      return Delegate->Initialize();
    }

    virtual void Finalize()
    {
      return Delegate->Finalize();
    }

    virtual void Suspend()
    {
      return Delegate->Suspend();
    }

    virtual void Resume()
    {
      return Delegate->Resume();
    }

    virtual void Execute(Async::Scheduler& sch)
    {
      while (!Delegate->IsFinished())
      {
        Delegate->ExecuteCycle();
        sch.Yield();
      }
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