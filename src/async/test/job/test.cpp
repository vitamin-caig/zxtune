#include <tools.h>
#include <async/job.h>
#include <boost/thread/thread.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

#define FILE_TAG 8D106607

namespace
{
	void ShowError(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
	{
		std::cout << Error::AttributesToString(loc, code, text);
	}
}

namespace
{
	using namespace Async;
	
	Error FailedToInitializeError()
  {
    return Error(THIS_LINE, 1, "Failed to initialize");
  }

	Error FailedToFinalizeError()
  {
    return Error(THIS_LINE, 2, "Failed to finalize");
  }

	Error FailedToSuspendError()
  {
    return Error(THIS_LINE, 3, "Failed to suspend");
  }

	Error FailedToResumeError()
  {
    return Error(THIS_LINE, 4, "Failed to resume");
  }

	Error FailedToExecuteError()
  {
    return Error(THIS_LINE, 5, "Failed to execute");
  }

	class TempWorker : public Job::Worker
	{
	public:
    TempWorker()
      : Finished(true)
    {
    }
    
    virtual Error Initialize()
    {
#ifndef NDEBUG
      std::cerr << " " << __FUNCTION__ " called (" << InitError << ")" << std::endl;
#endif
      return InitError;
    }
    
    virtual Error Finalize()
    {
#ifndef NDEBUG
      std::cerr << " " << __FUNCTION__ " called (" << FinalError << ")" << std::endl;
#endif
      return FinalError;
    }

    virtual Error Suspend()
    {
#ifndef NDEBUG
      std::cerr << " " << __FUNCTION__ " called (" << SuspendError << ")" << std::endl;
#endif
      return SuspendError;
    }
    
    virtual Error Resume()
    {
#ifndef NDEBUG
      std::cerr << " " << __FUNCTION__ " called (" << ResumeError << ")" << std::endl;
#endif
      return ResumeError;
    }

    virtual Error ExecuteCycle()
    {
#ifndef NDEBUG
      std::cerr << " " << __FUNCTION__ " called (" << ExecError << ")" << std::endl;
#endif
      return ExecError;
    }
    
    virtual bool IsFinished() const
    {
#ifndef NDEBUG
      std::cerr << " " << __FUNCTION__ " called (" << Finished << ")" << std::endl;
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
      throw Error(loc, 1000, "Job should be active");
    }
  }

  void CheckNotActive(const Job& job, Error::LocationRef loc)
  {
    if (job.IsActive())
    {
      throw Error(loc, 1001, "Job should not be active");
    }
  }

  void CheckPaused(const Job& job, Error::LocationRef loc)
  {
    if (!job.IsPaused())
    {
      throw Error(loc, 1002, "Job should be paused");
    }
  }

  void CheckNotPaused(const Job& job, Error::LocationRef loc)
  {
    if (job.IsPaused())
    {
      throw Error(loc, 1003, "Job should not be paused");
    }
  }

	void TestInvalidWorker()
	{
		std::cout << "Test for invalid worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    const Error& initErr = FailedToInitializeError();
    worker.InitError = initErr;
    if (const Error& err = job->Start())
    {
      if (err != initErr)
      {
        throw Error(THIS_LINE, 100, "Invalid error returned for initialize").AddSuberror(err);
      }
    }
		else
		{
			throw Error(THIS_LINE, 101, "Should not start job");
		}
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestNoCycleWorker()
  {
    std::cout << "Test for no cycle worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    const Error& execErr = FailedToExecuteError();
    worker.ExecError = execErr;
    ThrowIfError(job->Start());
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CheckNotActive(*job, THIS_LINE);
    ThrowIfError(job->Stop());
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorCycleWorker()
  {
    std::cout << "Test for first cycle error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    const Error& execErr = FailedToExecuteError();
    worker.ExecError = execErr;
    ThrowIfError(job->Start());
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CheckNotActive(*job, THIS_LINE);
    if (const Error& err = job->Stop())
    {
      if (err != execErr)
      {
        throw Error(THIS_LINE, 103, "Invalid error returned for stop failed").AddSuberror(err);
      }
    }
    else
    {
      throw Error(THIS_LINE, 104, "Should not stop failed job");
    }
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
	}

	void TestErrorStopWorker()
  {
    std::cout << "Test for deinit error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    const Error& finErr = FailedToFinalizeError();
    worker.FinalError = finErr;
    ThrowIfError(job->Start());
    CheckActive(*job, THIS_LINE);
    if (const Error& err = job->Stop())
    {
      if (err != finErr)
      {
        throw Error(THIS_LINE, 103, "Invalid error returned for stop failed").AddSuberror(err);
      }
    }
    else
    {
      throw Error(THIS_LINE, 104, "Should not stop failed job");
    }
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorSuspendWorker()
  {
    std::cout << "Test for suspend error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    const Error& suspErr = FailedToSuspendError();
    worker.SuspendError = suspErr;
    ThrowIfError(job->Start());
    CheckActive(*job, THIS_LINE);
    if (const Error& err = job->Pause())
    {
      if (err != suspErr)
      {
        throw Error(THIS_LINE, 103, "Invalid error returned for pause failed").AddSuberror(err);
      }
    }
    else
    {
      throw Error(THIS_LINE, 104, "Should not pause failed job");
    }
    CheckNotActive(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestErrorResumeWorker()
  {
    std::cout << "Test for resume error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    const Error& resErr = FailedToResumeError();
    worker.ResumeError = resErr;
    ThrowIfError(job->Start());
    CheckActive(*job, THIS_LINE);
    ThrowIfError(job->Pause());
    CheckActive(*job, THIS_LINE);
    if (const Error& err = job->Start())
    {
      if (err != resErr)
      {
        throw Error(THIS_LINE, 103, "Invalid error returned for resume failed").AddSuberror(err);
      }
    }
    else
    {
      throw Error(THIS_LINE, 104, "Should not resume failed job");
    }
    CheckNotActive(*job, THIS_LINE);
    ThrowIfError(job->Start());
    CheckActive(*job, THIS_LINE);
    ThrowIfError(job->Pause());
    CheckActive(*job, THIS_LINE);
    if (const Error& err = job->Stop())
    {
      if (err != resErr)
      {
        throw Error(THIS_LINE, 103, "Invalid error returned for resume while stopping failed").AddSuberror(err);
      }
    }
    else
    {
      throw Error(THIS_LINE, 104, "Job::Worker::Resume is not called");
    }
    CheckNotActive(*job, THIS_LINE);

    std::cout << "Succeed\n";
  }

  void TestSimpleWorkflow()
  {
    std::cout << "Test for simple workflow" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    CheckNotActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    ThrowIfError(job->Start());
    CheckActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    ThrowIfError(job->Pause());
    CheckActive(*job, THIS_LINE);
    CheckPaused(*job, THIS_LINE);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    ThrowIfError(job->Stop());
    CheckNotActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestDoubleWorkflow()
  {
    std::cout << "Test for double workflow" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    for (int i = 1; i < 2; ++i)
    {
      CheckNotActive(*job, THIS_LINE);
      CheckNotPaused(*job, THIS_LINE);
      ThrowIfError(job->Start());
      CheckActive(*job, THIS_LINE);
      CheckNotPaused(*job, THIS_LINE);
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
      ThrowIfError(job->Pause());
      CheckActive(*job, THIS_LINE);
      CheckPaused(*job, THIS_LINE);
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
      ThrowIfError(job->Stop());
    }
    CheckNotActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    std::cout << "Succeed\n";
  }

  void TestDuplicatedCalls()
  {
    std::cout << "Test for duplicated calls" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    CheckNotActive(*job, THIS_LINE);
    CheckNotPaused(*job, THIS_LINE);
    for (int i = 1; i < 2; ++i)
    {
      ThrowIfError(job->Start());
      CheckActive(*job, THIS_LINE);
      CheckNotPaused(*job, THIS_LINE);
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }
    for (int i = 1; i < 2; ++i)
    {
      ThrowIfError(job->Pause());
      CheckActive(*job, THIS_LINE);
      CheckPaused(*job, THIS_LINE);
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }
    for (int i = 1; i < 2; ++i)
    {
      ThrowIfError(job->Stop());
      CheckNotActive(*job, THIS_LINE);
      CheckNotPaused(*job, THIS_LINE);
    }
    std::cout << "Succeed\n";
  }
}

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
  }
  catch (const Error& err)
  {
		std::cout << "Failed: \n";
		err.WalkSuberrors(ShowError);
  }
}