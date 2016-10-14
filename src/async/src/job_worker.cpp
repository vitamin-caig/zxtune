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

    void Initialize() override
    {
      return Delegate->Initialize();
    }

    void Finalize() override
    {
      return Delegate->Finalize();
    }

    void Suspend() override
    {
      return Delegate->Suspend();
    }

    void Resume() override
    {
      return Delegate->Resume();
    }

    void Execute(Scheduler& sch) override
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
