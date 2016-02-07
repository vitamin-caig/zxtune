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
#include <make_ptr.h>
//library includes
#include <async/coroutine.h>
#include <async/worker.h>

namespace Async
{
  class WorkerCoroutine : public Coroutine
  {
  public:
    explicit WorkerCoroutine(Worker::Ptr worker)
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

    virtual void Execute(Scheduler& sch)
    {
      while (!Delegate->IsFinished())
      {
        Delegate->ExecuteCycle();
        sch.Yield();
      }
    }
  private:
    const Worker::Ptr Delegate;
  };
}

namespace Async
{
  Job::Ptr CreateJob(Worker::Ptr worker)
  {
    const Coroutine::Ptr routine = MakePtr<WorkerCoroutine>(worker);
    return CreateJob(routine);
  }
}
