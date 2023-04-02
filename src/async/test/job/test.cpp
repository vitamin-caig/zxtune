/**
 *
 * @file
 *
 * @brief Job test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <async/worker.h>
#include <error.h>
#include <iostream>
#include <pointers.h>
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
    return Error(THIS_LINE, "Failed to initialize");
  }

  Error FailedToFinalizeError()
  {
    return Error(THIS_LINE, "Failed to finalize");
  }

  Error FailedToSuspendError()
  {
    return Error(THIS_LINE, "Failed to suspend");
  }

  Error FailedToResumeError()
  {
    return Error(THIS_LINE, "Failed to resume");
  }

  Error FailedToExecuteError()
  {
    return Error(THIS_LINE, "Failed to execute");
  }

  class TempWorker : public Worker
  {
  public:
    TempWorker()
      : Finished(true)
    {}

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
    bool Finished;
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

  void TestInvalidWorker()
  {
    std::cout << "Test for invalid worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    const Error& initErr = FailedToInitializeError();
    worker.InitError = initErr;
    try
    {
      job->Start();
      throw Error(THIS_LINE, "Should not start job");
    }
    catch (const Error& err)
    {
      if (err != initErr)
      {
        throw Error(THIS_LINE, "Invalid error returned for initialize").AddSuberror(err);
      }
    }
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestNoCycleWorker()
  {
    std::cout << "Test for no cycle worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    const Error& execErr = FailedToExecuteError();
    worker.ExecError = execErr;
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
    const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    const Error& execErr = FailedToExecuteError();
    worker.ExecError = execErr;
    job->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CheckNotActive(*job, THIS_LINE);
    try
    {
      job->Stop();
      throw Error(THIS_LINE, "Should not stop failed job");
    }
    catch (const Error& err)
    {
      if (err != execErr)
      {
        throw Error(THIS_LINE, "Invalid error returned for stop failed").AddSuberror(err);
      }
    }
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorStopWorker()
  {
    std::cout << "Test for deinit error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    const Error& finErr = FailedToFinalizeError();
    worker.FinalError = finErr;
    job->Start();
    CheckActive(*job, THIS_LINE);
    try
    {
      job->Stop();
      throw Error(THIS_LINE, "Should not stop failed job");
    }
    catch (const Error& err)
    {
      if (err != finErr)
      {
        throw Error(THIS_LINE, "Invalid error returned for stop failed").AddSuberror(err);
      }
    }
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorSuspendWorker()
  {
    std::cout << "Test for suspend error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    const Error& suspErr = FailedToSuspendError();
    worker.SuspendError = suspErr;
    job->Start();
    CheckActive(*job, THIS_LINE);
    try
    {
      job->Pause();
      throw Error(THIS_LINE, "Should not pause failed job");
    }
    catch (const Error& err)
    {
      if (err != suspErr)
      {
        throw Error(THIS_LINE, "Invalid error returned for pause failed").AddSuberror(err);
      }
    }
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorResumeWorker()
  {
    std::cout << "Test for resume error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
    worker.Finished = false;
    const Error& resErr = FailedToResumeError();
    worker.ResumeError = resErr;
    job->Start();
    CheckActive(*job, THIS_LINE);
    job->Pause();
    CheckActive(*job, THIS_LINE);
    try
    {
      job->Start();
      throw Error(THIS_LINE, "Should not resume failed job");
    }
    catch (const Error& err)
    {
      if (err != resErr)
      {
        throw Error(THIS_LINE, "Invalid error returned for resume failed").AddSuberror(err);
      }
    }
    CheckNotActive(*job, THIS_LINE);
    job->Start();
    CheckActive(*job, THIS_LINE);
    job->Pause();
    CheckActive(*job, THIS_LINE);
    try
    {
      job->Stop();
      throw Error(THIS_LINE, "Job::Worker::Resume is not called");
    }
    catch (const Error& err)
    {
      if (err != resErr)
      {
        throw Error(THIS_LINE, "Invalid error returned for resume while stopping failed").AddSuberror(err);
      }
    }
    CheckNotActive(*job, THIS_LINE);

    std::cout << "Succeed\n";
  }

  void TestSimpleWorkflow()
  {
    std::cout << "Test for simple workflow" << std::endl;
    TempWorker worker;
    const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
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
    const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
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
    const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
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
      const Job::Ptr job = CreateJob(Worker::Ptr(&worker, NullDeleter<Worker>()));
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
  }
}
