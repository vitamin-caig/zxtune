/**
 *
 * @file
 *
 * @brief Worker-based Job factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
// library includes
#include <async/coroutine.h>
#include <async/worker.h>
// std includes
#include <utility>

namespace Async
{
  class WorkerCoroutine : public Coroutine
  {
  public:
    explicit WorkerCoroutine(Worker::Ptr worker)
      : Delegate(std::move(worker))
    {}

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
}  // namespace Async

namespace Async
{
  Job::Ptr CreateJob(Worker::Ptr worker)
  {
    auto routine = MakePtr<WorkerCoroutine>(std::move(worker));
    return CreateJob(std::move(routine));
  }
}  // namespace Async
