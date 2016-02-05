/**
* 
* @file
*
* @brief Coroutine-based Job implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "event.h"
//common includes
#include <make_ptr.h>
//library includes
#include <async/activity.h>
#include <async/coroutine.h>

namespace
{
  void PausePaused()
  {
  }

  void PauseStopped()
  {
  }

  void StopStopped()
  {
  }

  void StartStarted()
  {
  }

  enum JobState
  {
    STOPPED,
    STOPPING,
    STARTED,
    STARTING,
    PAUSED,
    PAUSING,
  };

  struct StoppingEvent {};

  class CoroutineOperation : public Async::Operation
                           , private Async::Scheduler
  {
  public:
    CoroutineOperation(Async::Coroutine::Ptr routine, Async::Event<JobState>& state)
      : Routine(routine)
      , State(state)
    {
    }

    virtual void Prepare()
    {
      return Routine->Initialize();
    }
    
    virtual void Execute()
    {
      try
      {
        Routine->Execute(*this);
        Finalize();
      }
      catch (const StoppingEvent&)
      {
        Finalize();
      }
      catch (const Error&)
      {
        Finalize();
        throw;
      }
    }
  private:
    void Finalize()
    {
      State.Set(STOPPED);
      Routine->Finalize();
    }

    virtual void Yield()
    {
      switch (State.WaitForAny(STOPPING, PAUSING, STARTED))
      {
      case PAUSING:
        {
          Routine->Suspend();
          State.Set(PAUSED);
          const JobState nextState = State.WaitForAny(STOPPING, STARTING);
          Routine->Resume();
          if (STARTING == nextState)
          {
            State.Set(STARTED);
            break;
          }
        }
      case STOPPING:
        throw StoppingEvent();
      default:
        break;
      }
    }
  private:
    const Async::Coroutine::Ptr Routine;
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

    virtual void Start()
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
      const Async::Operation::Ptr jobOper = MakePtr<CoroutineOperation>(Routine, boost::ref(State));
      Act = Async::Activity::Create(jobOper);
      State.Set(STARTED);
    }
    
    virtual void Pause()
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
      return PauseStopped();
    }
    
    virtual void Stop()
    {
      const boost::mutex::scoped_lock lock(Mutex);
      if (Act)
      {
        return FinishAction();//TODO: wrap error
      }
      return StopStopped();
    }

    virtual bool IsActive() const
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Act && Act->IsExecuted();
    }
    
    virtual bool IsPaused() const
    {
      const boost::mutex::scoped_lock lock(Mutex);
      return Act && Act->IsExecuted() && State.Check(PAUSED);
    }
  private:
    void StartExecutingAction()
    {
      if (State.Check(PAUSED))
      {
        State.Set(STARTING);
        if (STOPPED == State.WaitForAny(STOPPED, STARTED))
        {
          return FinishAction();
        }
      }
      else
      {
        return StartStarted();
      }
    }

    void PauseExecutingAction()
    {
      if (!State.Check(PAUSED))
      {
        State.Set(PAUSING);
        if (STOPPED == State.WaitForAny(STOPPED, PAUSED))
        {
          return FinishAction();
        }
      }
      else
      {
        return PausePaused();
      }
    }
    
    void FinishAction()
    {
      State.Set(STOPPING);
      try
      {
        Async::Activity::Ptr act;
        act.swap(Act);
        act->Wait();
        State.Reset();
      }
      catch (const Error& /*err*/)
      {
        State.Reset();
      }
    }
  private:
    const Async::Coroutine::Ptr Routine;
    mutable boost::mutex Mutex;
    Async::Event<JobState> State;
    Async::Activity::Ptr Act;
  };
}

namespace Async
{
  Job::Ptr CreateJob(Coroutine::Ptr routine)
  {
    return MakePtr<JobImpl>(routine);
  }
}