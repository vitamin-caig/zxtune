/*
Abstract:
  Asynchronous activity implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <async/activity.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>

namespace
{
  using namespace Async;

  template<class Type>
  class Event
  {
  public:
    explicit Event(Type val)
      : Value(1 << val)
    {
    }

    Event()
      : Value(0)
    {
    }

    void Set(Type val)
    {
      {
        boost::mutex::scoped_lock lock(Mutex);
        Value = 1 << val;
      }
      Condition.notify_all();
    }

    void Wait(Type val)
    {
      WaitInternal(1 << val);
    }

    Type WaitForAny(Type val1, Type val2)
    {
      const uint32_t mask1 = 1 << val1;
      const uint32_t mask2 = 1 << val2;
      const uint32_t res = WaitInternal(mask1 | mask2);
      return (res & mask1) ? val1 : val2;
    }

    bool Check(Type val) const
    {
      boost::mutex::scoped_lock lock(Mutex);
      return IsSignalled(1 << val);
    }
  private:
    uint32_t WaitInternal(uint32_t mask)
    {
      boost::mutex::scoped_lock lock(Mutex);
      Condition.wait(lock, boost::bind(&Event::IsSignalled, this, mask));
      return Value;
    }

    bool IsSignalled(uint32_t mask) const
    {
      return 0 != (Value & mask);
    }
  private:
    uint32_t Value;
    mutable boost::mutex Mutex;
    boost::condition_variable Condition;
  };

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
}

namespace Async
{
  Error Activity::Create(Operation::Ptr operation, Activity::Ptr& result)
  {
    const ActivityImpl::Ptr activity(new ActivityImpl(operation));
    if (const Error& err = activity->Start())
    {
      return err;
    }
    result = activity;
    return Error();
  }
}
