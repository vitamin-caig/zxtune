/**
 *
 * @file
 *
 * @brief Asynchronous activity implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "async/activity.h"

#include "async/src/event.h"

#include "error.h"
#include "make_ptr.h"
#include "pointers.h"

#include <cassert>
#include <thread>
#include <utility>

namespace Async
{
  enum class ActivityState
  {
    STOPPED,
    INITIALIZED,
    FAILED,
    STARTED
  };

  class ThreadActivity : public Activity
  {
  public:
    using Ptr = std::shared_ptr<ThreadActivity>;

    explicit ThreadActivity(Operation::Ptr op)
      : Oper(std::move(op))
      , State(ActivityState::STOPPED)
    {}

    ~ThreadActivity() override
    {
      assert(!IsExecuted() || !"Should call Activity::Wait before stop");
    }

    void Start()
    {
      Thread = std::thread(&ThreadActivity::WorkProc, this);
      if (ActivityState::FAILED == State.WaitForAny(ActivityState::INITIALIZED, ActivityState::FAILED))
      {
        Thread.join();
        State.Set(ActivityState::STOPPED);
        throw LastError;
      }
      State.Set(ActivityState::STARTED);
    }

    bool IsExecuted() const override
    {
      return State.Check(ActivityState::STARTED);
    }

    void Wait() override
    {
      if (Thread.joinable())
      {
        Thread.join();
      }
      ThrowIfError(LastError);
    }

  private:
    void WorkProc()
    {
      LastError = Error();
      try
      {
        Oper->Prepare();
        State.Set(ActivityState::INITIALIZED);
        State.Wait(ActivityState::STARTED);
        Oper->Execute();
        State.Set(ActivityState::STOPPED);
      }
      catch (const Error& err)
      {
        LastError = err;
        State.Set(ActivityState::FAILED);
      }
    }

  private:
    const Operation::Ptr Oper;
    Event<ActivityState> State;
    std::thread Thread;
    Error LastError;
  };

  class StubActivity : public Activity
  {
  public:
    bool IsExecuted() const override
    {
      return false;
    }

    void Wait() override {}
  };
}  // namespace Async

namespace Async
{
  Activity::Ptr Activity::Create(Operation::Ptr operation)
  {
    auto result = MakePtr<ThreadActivity>(std::move(operation));
    result->Start();
    return result;
  }

  Activity::Ptr Activity::CreateStub()
  {
    static StubActivity stub;
    return MakeSingletonPointer(stub);
  }
}  // namespace Async
