/*
Abstract:
  Asynchronous job implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "event.h"
//library includes
#include <async/activity.h>
#include <async/coroutine.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  Error BuildPausePausedError()
  {
    return Error();
  }

  Error BuildPauseStoppedError()
  {
    return Error();
  }

  Error BuildStopStoppedError()
  {
    return Error();
  }

  Error BuildStartStartedError()
  {
    return Error();
  }

  enum JobState
  {
    STOP,
    PAUSE,
    START
  };

  struct ErrorObject
  {
    Error Err;

    ErrorObject()
    {
    }

    explicit ErrorObject(const Error& e)
      : Err(e)
    {
    }
  };

  void TransportError(const Error& e)
  {
    if (e)
    {
      throw ErrorObject(e);
    }
  }

  class CoroutineOperation : public Async::Operation
                           , private Async::Scheduler
  {
  public:
    CoroutineOperation(Async::Coroutine::Ptr routine, Async::Event<JobState>& signal, Async::Event<JobState>& state)
      : Routine(routine)
      , Signal(signal)
      , State(state)
    {
    }

    virtual Error Prepare()
    {
      return Routine->Initialize();
    }
    
    virtual Error Execute()
    {
      State.Set(START);
      Error lastError = ExecuteRoutine();
      State.Set(STOP);
      if (Error err = Routine->Finalize())
      {
        return err.AddSuberror(lastError);
      }
      return lastError;
    }
  private:
    Error ExecuteRoutine()
    {
      try
      {
        return Routine->Execute(*this);
      }
      catch (const ErrorObject& e)
      {
        return e.Err;
      }
    }

    virtual void Yield()
    {
      switch (Signal.WaitForAny(STOP, PAUSE, START))
      {
      case STOP:
        throw ErrorObject();
      case PAUSE:
        TransportError(Routine->Suspend());
        State.Set(PAUSE);
        Signal.WaitForAny(STOP, START);
        TransportError(Routine->Resume());
        State.Set(START);
        break;
      default:
        break;
      }
    }
  private:
    const Async::Coroutine::Ptr Routine;
    Async::Event<JobState>& Signal;
    Async::Event<JobState>& State;
  };
  
  class JobImpl : public Async::Job
  {
  public:
    explicit JobImpl(Async::Coroutine::Ptr routine)
      : Routine(routine)
    {
    }

    virtual ~JobImpl()
    {
      if (Act)
      {
        FinishAction();
      }
    }

    virtual Error Start()
    {
      try
      {
        const boost::mutex::scoped_lock lock(Mutex);
        if (Act)
        {
          if (Act->IsExecuted())
          {
            return StartExecutingAction();
          }
          FinishAction();
        }
        const Async::Operation::Ptr jobOper = boost::make_shared<CoroutineOperation>(Routine, boost::ref(Signal), boost::ref(State));
        Act = Async::Activity::Create(jobOper);
        return Error();
      }
      catch (const Error& err)
      {
        return err;
      }
    }
    
    virtual Error Pause()
    {
      const boost::mutex::scoped_lock lock(Mutex);
      if (Act)
      {
        if (Act->IsExecuted())
        {
          return PauseExecutingAction();
        }
        return FinishAction();
      }
      return BuildPauseStoppedError();
    }
    
    virtual Error Stop()
    {
      const boost::mutex::scoped_lock lock(Mutex);
      if (Act)
      {
        return FinishAction();//TODO: wrap error
      }
      return BuildStopStoppedError();
    }

    virtual bool IsActive() const
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Act && Act->IsExecuted();
    }
    
    virtual bool IsPaused() const
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Act && Act->IsExecuted() && State.Check(PAUSE);
    }
  private:
    Error StartExecutingAction()
    {
      if (State.Check(PAUSE))
      {
        Signal.Set(START);
        if (STOP == State.WaitForAny(STOP, START))
        {
          return FinishAction();
        }
        return Error();
      }
      return BuildStartStartedError();
    }

    Error PauseExecutingAction()
    {
      if (!State.Check(PAUSE))
      {
        Signal.Set(PAUSE);
        if (STOP == State.WaitForAny(STOP, PAUSE))
        {
          return FinishAction();
        }
        return Error();
      }
      return BuildPausePausedError();
    }
    
    Error FinishAction()
    {
      Signal.Set(STOP);
      const Error err = Act->Wait();
      Act.reset();
      Signal.Reset();
      State.Reset();
      return err;
    }
  private:
    const Async::Coroutine::Ptr Routine;
    mutable boost::mutex Mutex;
    Async::Event<JobState> Signal;
    Async::Event<JobState> State;
    Async::Activity::Ptr Act;
  };
}

namespace Async
{
  Job::Ptr CreateJob(Coroutine::Ptr routine)
  {
    return boost::make_shared<JobImpl>(routine);
  }
}