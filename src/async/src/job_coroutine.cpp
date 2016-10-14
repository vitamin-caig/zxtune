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

namespace Async
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

  class CoroutineOperation : public Operation
                           , private Scheduler
  {
  public:
    CoroutineOperation(Coroutine::Ptr routine, Event<JobState>& state)
      : Routine(routine)
      , State(state)
    {
    }

    void Prepare() override
    {
      return Routine->Initialize();
    }
    
    void Execute() override
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

    void Yield() override
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
    const Coroutine::Ptr Routine;
    Event<JobState>& State;
  };
  
  class CoroutineJob : public Job
  {
  public:
    explicit CoroutineJob(Coroutine::Ptr routine)
      : Routine(routine)
    {
    }

    ~CoroutineJob() override
    {
      if (Act)
      {
        FinishAction();
      }
    }

    void Start() override
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      if (Act)
      {
        if (Act->IsExecuted())
        {
          return StartExecutingAction();
        }
        FinishAction();
      }
      const Operation::Ptr jobOper = MakePtr<CoroutineOperation>(Routine, State);
      Act = Activity::Create(jobOper);
      State.Set(STARTED);
    }
    
    void Pause() override
    {
      const std::lock_guard<std::mutex> lock(Mutex);
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
    
    void Stop() override
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      if (Act)
      {
        return FinishAction();//TODO: wrap error
      }
      return StopStopped();
    }

    bool IsActive() const override
    {
      const std::lock_guard<std::mutex> lock(Mutex);
      return Act && Act->IsExecuted();
    }
    
    bool IsPaused() const override
    {
      const std::lock_guard<std::mutex> lock(Mutex);
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
        Activity::Ptr act;
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
    const Coroutine::Ptr Routine;
    mutable std::mutex Mutex;
    Event<JobState> State;
    Activity::Ptr Act;
  };
}

namespace Async
{
  Job::Ptr CreateJob(Coroutine::Ptr routine)
  {
    return MakePtr<CoroutineJob>(routine);
  }
}
