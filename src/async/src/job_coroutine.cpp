/**
 *
 * @file
 *
 * @brief Coroutine-based Job implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "async/src/event.h"

#include <async/activity.h>
#include <async/coroutine.h>

#include <make_ptr.h>

#include <utility>

namespace Async
{
  void PausePaused() {}

  void PauseStopped() {}

  void StopStopped() {}

  void StartStarted() {}

  enum class JobState
  {
    STOPPED,
    STOPPING,
    STARTED,
    STARTING,
    PAUSED,
    PAUSING,
  };

  struct StoppingEvent
  {};

  class CoroutineOperation
    : public Operation
    , private Scheduler
  {
  public:
    CoroutineOperation(Coroutine::Ptr routine, Event<JobState>& state)
      : Routine(std::move(routine))
      , State(state)
    {}

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
      State.Set(JobState::STOPPED);
      Routine->Finalize();
    }

    void Yield() override
    {
      switch (State.WaitForAny(JobState::STOPPING, JobState::PAUSING, JobState::STARTED))
      {
      case JobState::PAUSING:
      {
        Routine->Suspend();
        State.Set(JobState::PAUSED);
        const JobState nextState = State.WaitForAny(JobState::STOPPING, JobState::STARTING);
        Routine->Resume();
        if (JobState::STARTING == nextState)
        {
          State.Set(JobState::STARTED);
          break;
        }
        else
        {
          throw StoppingEvent();
        }
      }
      case JobState::STOPPING:
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
      : Routine(std::move(routine))
    {}

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
      State.Set(JobState::STARTED);
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
        return FinishAction();  // TODO: wrap error
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
      return Act && Act->IsExecuted() && State.Check(JobState::PAUSED);
    }

  private:
    void StartExecutingAction()
    {
      if (State.Check(JobState::PAUSED))
      {
        State.Set(JobState::STARTING);
        if (JobState::STOPPED == State.WaitForAny(JobState::STOPPED, JobState::STARTED))
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
      if (!State.Check(JobState::PAUSED))
      {
        State.Set(JobState::PAUSING);
        if (JobState::STOPPED == State.WaitForAny(JobState::STOPPED, JobState::PAUSED))
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
      State.Set(JobState::STOPPING);
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
}  // namespace Async

namespace Async
{
  Job::Ptr CreateJob(Coroutine::Ptr routine)
  {
    return MakePtr<CoroutineJob>(std::move(routine));
  }
}  // namespace Async
