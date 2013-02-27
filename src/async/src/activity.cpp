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

    void Start()
    {
      Thread = boost::thread(std::mem_fun(&ActivityImpl::WorkProc), this);
      if (FAILED == State.WaitForAny(INITIALIZED, FAILED))
      {
        Thread.join();
        State.Set(STOPPED);
        throw LastError;
      }
      State.Set(STARTED);
    }

    virtual bool IsExecuted() const
    {
      return State.Check(STARTED);
    }

    virtual void Wait()
    {
      Thread.join();
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
    boost::thread Thread;
    Error LastError;
  };

  class StubActivity : public Async::Activity
  {
  public:
    virtual bool IsExecuted() const
    {
      return false;
    }

    virtual void Wait()
    {
    }
  };
}

namespace Async
{
  Activity::Ptr Activity::Create(Operation::Ptr operation)
  {
    const ActivityImpl::Ptr result = boost::make_shared<ActivityImpl>(operation);
    result->Start();
    return result;
  }

  Activity::Ptr Activity::CreateStub()
  {
    return boost::make_shared<StubActivity>();
  }
}
