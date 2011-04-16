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
#include <async/job.h>
#include <async/activity.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace Async;

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

  class JobOperation : public Operation
  {
  public:
    JobOperation(Job::Worker::Ptr worker, Event<JobState>& signal, Event<JobState>& state)
      : Work(worker)
      , Signal(signal)
      , State(state)
    {
    }

    virtual Error Prepare()
    {
      return Work->Initialize();//TODO: wrap error
    }
    
    virtual Error Execute()
    {
      Error lastError;
      while (!Work->IsFinished())
      {
        const JobState signal = Signal.WaitForAny(STOP, PAUSE, START);
        if (STOP == signal)
        {
          break;
        }
        else if (PAUSE == signal)
        {
          if (lastError = Work->Suspend())
          {
            break;
          }
          State.Set(PAUSE);
          Signal.WaitForAny(STOP, START);
          if (lastError = Work->Resume())
          {
            break;
          }
          State.Set(START);
        }
        else if (lastError = Work->ExecuteCycle())
        {
          break;
        }
      }
      State.Set(STOP);
      if (Error err = Work->Finalize())
      {
        return err.AddSuberror(lastError);
      }
      return lastError;
    }
  private:
    const Job::Worker::Ptr Work;
    Event<JobState>& Signal;
    Event<JobState>& State;
  };
  
  class JobImpl : public Job
  {
  public:
    explicit JobImpl(Job::Worker::Ptr worker)
      : Work(worker)
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
      const boost::mutex::scoped_lock lock(Mutex);
      if (Act)
      {
        if (Act->IsExecuted())
        {
          return StartExecutingAction();
        }
        FinishAction();
      }
      const Operation::Ptr jobOper = boost::make_shared<JobOperation>(Work, boost::ref(Signal), boost::ref(State));
      if (const Error& err = Activity::Create(jobOper, Act))
      {
        return err;//TODO: wrap
      }
      Signal.Set(START);
      return Error();
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
        if (Act->IsExecuted())
        {
          Signal.Set(STOP);
        }
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
      return err;
    }
  private:
    const Job::Worker::Ptr Work;
    mutable boost::mutex Mutex;
    Event<JobState> Signal;
    Event<JobState> State;
    Activity::Ptr Act;
  };
}

namespace Async
{
  Job::Ptr Job::Create(Job::Worker::Ptr worker)
  {
    return boost::make_shared<JobImpl>(worker);
  }
}