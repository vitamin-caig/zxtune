/**
* 
* @file
*
* @brief Asynchronous activity implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "event.h"
//common includes
#include <pointers.h>
#include <make_ptr.h>
//library includes
#include <async/activity.h>
//std includes
#include <cassert>
#include <thread>

namespace Async
{
  enum ActivityState
  {
    STOPPED,
    INITIALIZED,
    FAILED,
    STARTED
  };

  class ThreadActivity : public Activity
  {
  public:
    typedef std::shared_ptr<ThreadActivity> Ptr;

    explicit ThreadActivity(Operation::Ptr op)
      : Oper(op)
      , State(STOPPED)
    {
    }

    ~ThreadActivity() override
    {
      assert(!IsExecuted() || !"Should call Activity::Wait before stop");
    }

    void Start()
    {
      Thread = std::thread(std::mem_fun(&ThreadActivity::WorkProc), this);
      if (FAILED == State.WaitForAny(INITIALIZED, FAILED))
      {
        Thread.join();
        State.Set(STOPPED);
        throw LastError;
      }
      State.Set(STARTED);
    }

    bool IsExecuted() const override
    {
      return State.Check(STARTED);
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
        State.Set(INITIALIZED);
        State.Wait(STARTED);
        Oper->Execute();
        State.Set(STOPPED);
      }
      catch (const Error& err)
      {
        LastError = err;
        State.Set(FAILED);
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

    void Wait() override
    {
    }
  };
}

namespace Async
{
  Activity::Ptr Activity::Create(Operation::Ptr operation)
  {
    const ThreadActivity::Ptr result = MakePtr<ThreadActivity>(operation);
    result->Start();
    return result;
  }

  Activity::Ptr Activity::CreateStub()
  {
    static StubActivity stub;
    return MakeSingletonPointer(stub);
  }
}
