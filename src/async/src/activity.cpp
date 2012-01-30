/*
Abstract:
  Asynchronous activity implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "event.h"
//library includes
#include <async/activity.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>

namespace
{
  using namespace Async;

  enum ActivityState
  {
    STOPPED,
    INITIALIZED,
    FAILED,
    STARTED
  };

  class ActivityImpl : public Activity
  {
  public:
    typedef boost::shared_ptr<ActivityImpl> Ptr;

    explicit ActivityImpl(Operation::Ptr op)
      : Oper(op)
      , State(STOPPED)
    {
    }

    virtual ~ActivityImpl()
    {
      assert(!IsExecuted() || !"Should call Activity::Wait before stop");
    }

    Error Start()
    {
      Thread = boost::thread(std::mem_fun(&ActivityImpl::WorkProc), this);
      if (FAILED == State.WaitForAny(INITIALIZED, FAILED))
      {
        Thread.join();
        State.Set(STOPPED);
        return LastError;
      }
      State.Set(STARTED);
      return Error();
    }

    virtual bool IsExecuted() const
    {
      return State.Check(STARTED);
    }

    virtual Error Wait()
    {
      Thread.join();
      return LastError;
    }
  private:
    void WorkProc()
    {
      if (LastError = Oper->Prepare())
      {
        State.Set(FAILED);
        return;
      }
      State.Set(INITIALIZED);
      State.Wait(STARTED);
      LastError = Oper->Execute();
      State.Set(STOPPED);
    }
  private:
    const Operation::Ptr Oper;
    boost::thread Thread;
    Event<ActivityState> State;
    Error LastError;
  };

  class StubActivity : public Async::Activity
  {
  public:
    virtual bool IsExecuted() const
    {
      return false;
    }

    virtual Error Wait()
    {
      return Error();
    }
  };
}

namespace Async
{
  Activity::Ptr Activity::Create(Operation::Ptr operation)
  {
    const ActivityImpl::Ptr result = boost::make_shared<ActivityImpl>(operation);
    ThrowIfError(result->Start());
    return result;
  }

  Activity::Ptr Activity::CreateStub()
  {
    return boost::make_shared<StubActivity>();
  }
}
