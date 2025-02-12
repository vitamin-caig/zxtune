/**
 *
 * @file
 *
 * @brief Job test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "async/worker.h"

#include "error.h"
#include "pointers.h"

#include <iostream>
#include <thread>

bool operator!=(const Error& lh, const Error& rh)
{
  return lh.ToString() != rh.ToString();
}

std::ostream& operator<<(std::ostream& o, const Error& e)
{
  o << e.ToString();
  return o;
}

namespace
{
  using namespace Async;

  Error FailedToInitializeError()
  {
    return {THIS_LINE, "Failed to initialize"};
  }

  Error FailedToFinalizeError()
  {
    return {THIS_LINE, "Failed to finalize"};
  }

  Error FailedToSuspendError()
  {
    return {THIS_LINE, "Failed to suspend"};
  }

  Error FailedToResumeError()
  {
    return {THIS_LINE, "Failed to resume"};
  }

  Error FailedToExecuteError()
  {
    return {THIS_LINE, "Failed to execute"};
  }

  class TempWorker : public Worker
  {
  public:
    TempWorker() = default;

    void Initialize() override
    {
      std::cerr << " " << __FUNCTION__ << " called (" << InitError << ")" << std::endl;
      ThrowIfError(InitError);
    }

    void Finalize() override
    {
      std::cerr << " " << __FUNCTION__ << " called (" << FinalError << ")" << std::endl;
      ThrowIfError(FinalError);
    }

    void Suspend() override
    {
      std::cerr << " " << __FUNCTION__ << " called (" << SuspendError << ")" << std::endl;
      ThrowIfError(SuspendError);
    }

    void Resume() override
    {
      std::cerr << " " << __FUNCTION__ << " called (" << ResumeError << ")" << std::endl;
      ThrowIfError(ResumeError);
    }

    void ExecuteCycle() override
    {
#ifndef NDEBUG
      std::cerr << " " << __FUNCTION__ << " called (" << ExecError << ")" << std::endl;
#endif
      ThrowIfError(ExecError);
    }

    bool IsFinished() const override
    {
#ifndef NDEBUG
      std::cerr << " " << __FUNCTION__ << " called (" << Finished << ")" << std::endl;
#endif
      return Finished;
    }

    Error InitError;
    Error FinalError;
    Error SuspendError;
    Error ResumeError;
    Error ExecError;
    bool Finished = true;
  };

  void CheckActive(const Job& job, Error::LocationRef loc)
  {
    if (!job.IsActive())
    {
      throw Error(loc, "Job should be active");
    }
  }

  void CheckNotActive(const Job& job, Error::LocationRef loc)
  {
    if (job.IsActive())
    {
      throw Error(loc, "Job should not be active");
    }
  }

  void CheckPaused(const Job& job, Error::LocationRef loc)
  {
    if (!job.IsPaused())
    {
      throw Error(loc, "Job should be paused");
    }
  }

  void CheckNotPaused(const Job& job, Error::LocationRef loc)
  {
    if (job.IsPaused())
    {
      throw Error(loc, "Job should not be paused");
    }
  }

  template<class F>
  void CheckNoThrow(Error::LocationRef loc, F&& func)
  {
    try
    {
      func();
    }
    catch (const Error& err)
    {
      throw Error(loc, "Unexpected error thrown").AddSuberror(err);
    }
  }

  template<class F>
  void CheckThrow(Error::LocationRef loc, const Error& expected, F&& func)
  {
    try
    {
      func();
      throw Error(THIS_LINE, "No expected throw");
    }
    catch (const Error& err)
    {
      if (err != expected)
      {
        throw Error(loc, "Broken expectation").AddSuberror(err);
      }
    }
  }

  void TestInvalidWorker()
  {
    std::cout << "Test for invalid worker" << std::endl;
    TempWorker worker;
    const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.InitError = FailedToInitializeError();
    CheckThrow(THIS_LINE, worker.InitError, [&job]() { job->Start(); });
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestNoCycleWorker()
  {
    std::cout << "Test for no cycle worker" << std::endl;
    TempWorker worker;
    const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.ExecError = FailedToExecuteError();
    job->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CheckNotActive(*job, THIS_LINE);
    job->Stop();
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorCycleWorker()
  {
    std::cout << "Test for first cycle error worker" << std::endl;
    TempWorker worker;
    const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    worker.ExecError = FailedToExecuteError();
    job->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CheckNotActive(*job, THIS_LINE);
    // See @Async::StopStopped behaviour
    CheckNoThrow(THIS_LINE, [&job]() { job->Stop(); });
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorStopWorker()
  {
    std::cout << "Test for deinit error worker" << std::endl;
    TempWorker worker;
    const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    worker.FinalError = FailedToFinalizeError();
    job->Start();
    CheckActive(*job, THIS_LINE);
    // See @Async::StopStopped behaviour
    CheckNoThrow(THIS_LINE, [&job]() { job->Stop(); });
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorSuspendWorker()
  {
    std::cout << "Test for suspend error worker" << std::endl;
    TempWorker worker;
    const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    worker.SuspendError = FailedToSuspendError();
    job->Start();
    CheckActive(*job, THIS_LINE);
    // See @Async::PauseStopped behaviour
    CheckNoThrow(THIS_LINE, [&job]() { job->Pause(); });
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorResumeWorker()
  {
    std::cout << "Test for resume error worker" << std::endl;
    TempWorker worker;
    const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    worker.ResumeError = FailedToResumeError();
    job->Start();
    CheckActive(*job, THIS_LINE);
    job->Pause();
    CheckActive(*job, THIS_LINE);
    // See Async::CoroutineJob::FinishAction behaviour
    CheckNoThrow(THIS_LINE, [&job]() { job->Start(); });
    CheckNotActive(*job, THIS_LINE);
    job->Start();
    CheckActive(*job, THIS_LINE);
    job->Pause();
    CheckActive(*job, THIS_LINE);
    // See Async::CoroutineJob::FinishAction behaviour
    CheckNoThrow(THIS_LINE, [&job]() { job->Stop(); });
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestSimpleWorkflow()
  {
    std::cout << "Test for simple workflow" << std::endl;
    TempWorker worker;
    const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    CheckNotActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    job->Start();
    CheckActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    job->Pause();
    CheckActive(*job, THIS_LINE);
    CheckPaused(*job, THIS_LINE);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    job->Stop();
    CheckNotActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestDoubleWorkflow()
  {
    std::cout << "Test for double workflow" << std::endl;
    TempWorker worker;
    const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    for (int i = 1; i < 2; ++i)
    {
      CheckNotActive(*job, THIS_LINE);
      CheckNotPaused(*job, THIS_LINE);
      job->Start();
      CheckActive(*job, THIS_LINE);
      CheckNotPaused(*job, THIS_LINE);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      job->Pause();
      CheckActive(*job, THIS_LINE);
      CheckPaused(*job, THIS_LINE);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      job->Stop();
    }
    CheckNotActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestDuplicatedCalls()
  {
    std::cout << "Test for duplicated calls" << std::endl;
    TempWorker worker;
    const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    CheckNotActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    for (int i = 1; i < 2; ++i)
    {
      job->Start();
      CheckActive(*job, THIS_LINE);
      CheckNotPaused(*job, THIS_LINE);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    for (int i = 1; i < 2; ++i)
    {
      job->Pause();
      CheckActive(*job, THIS_LINE);
      CheckPaused(*job, THIS_LINE);
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    for (int i = 1; i < 2; ++i)
    {
      job->Stop();
      CheckNotActive(*job, THIS_LINE);
      CheckNotPaused(*job, THIS_LINE);
    }
    std::cout << "Succeed\n";
  }

  void TestNoStopCall()
  {
    std::cout << "Test for stopping on destruction" << std::endl;
    {
      TempWorker worker;
      const auto job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
      worker.Finished = false;
      job->Start();
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
    std::cout << "Succeed\n";
  }
}  // namespace

int main()
{
  try
  {
    TestInvalidWorker();
    TestNoCycleWorker();
    TestErrorCycleWorker();
    TestErrorStopWorker();
    TestErrorSuspendWorker();
    TestErrorResumeWorker();
    TestSimpleWorkflow();
    TestDoubleWorkflow();
    TestDuplicatedCalls();
    TestNoStopCall();
  }
  catch (const Error& err)
  {
    std::cout << "Failed: \n";
    std::cerr << err.ToString();
    return 1;
  }
}
